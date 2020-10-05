/**
 * Implementation of the FluxSurfaceAverager class which 
 * handles everything needed to carry out flux surface 
 * averages in DREAM. 
 * 
 * It is initialized (or refreshed) with Rebuild(), which should
 * be called after RadialGridGenerator has called 
 * SetReferenceMagneticFieldData(...).
 */

#include "FVM/Grid/FluxSurfaceAverager.hpp"
#include "gsl/gsl_errno.h"
using namespace std;
using namespace DREAM::FVM;


/**
 * Constructor.
 */
FluxSurfaceAverager::FluxSurfaceAverager(
    RadialGrid *g, bool geometryIsSymmetric, len_t ntheta_interp,
    interp_method i_method, quadrature_method q_method
) : rGrid(g), geometryIsSymmetric(geometryIsSymmetric), 
    ntheta_interp(ntheta_interp) {
    const gsl_interp_type *interpolationMethod;
    switch(i_method){
        case INTERP_LINEAR:
            interpolationMethod = gsl_interp_linear;
            break;
        case INTERP_STEFFEN:{
            if(ntheta_interp>2)
                interpolationMethod = gsl_interp_steffen;
            else
                interpolationMethod = gsl_interp_linear;
            break;
        }
        default:
            throw FVMException("Interpolation method '%d' not supported by FluxSurfaceAverager.", i_method);
    }

    InitializeQuadrature(q_method);

    B        = new FluxSurfaceQuantity(rGrid, interpolationMethod);
    Jacobian = new FluxSurfaceQuantity(rGrid, interpolationMethod);
    ROverR0  = new FluxSurfaceQuantity(rGrid, interpolationMethod);
    NablaR2  = new FluxSurfaceQuantity(rGrid, interpolationMethod);

    // Use the Brent algorithm for root finding in determining the theta bounce points
    const gsl_root_fsolver_type *GSL_rootsolver_type = gsl_root_fsolver_brent;
    gsl_fsolver = gsl_root_fsolver_alloc (GSL_rootsolver_type);
    qaws_table_passing = gsl_integration_qaws_table_alloc(0.0, 0.0, 0, 0);
    qaws_table_trapped = gsl_integration_qaws_table_alloc(-0.5, -0.5, 0, 0);
}

/**
 * Destructor
 */
FluxSurfaceAverager::~FluxSurfaceAverager(){
    gsl_root_fsolver_free(gsl_fsolver);

    DeallocateQuadrature();
    DeallocateReferenceData();
    delete B;
    delete Jacobian;
    delete ROverR0;
    delete NablaR2;

}


/**
 * (Re-)Initializes everyting required to perform flux surface averages.
 * Should be called after SetReferenceMagneticFieldData(...).
 */
void FluxSurfaceAverager::Rebuild(){
    this->nr = rGrid->GetNr();

    // if using fixed quadrature, store all quantities on the theta grid
    if(!integrateAdaptive){
        B->InterpolateMagneticDataToTheta(theta, ntheta_interp);
        Jacobian->InterpolateMagneticDataToTheta(theta, ntheta_interp);
        ROverR0->InterpolateMagneticDataToTheta(theta, ntheta_interp);
        NablaR2->InterpolateMagneticDataToTheta(theta, ntheta_interp);
    }

    // Calculate flux-surface averaged Jacobian and hand over to RadialGrid.
    function<real_t(real_t,real_t,real_t)> unityFunc 
               = [](real_t,real_t,real_t){return 1;};
    real_t *VpVol   = new real_t[nr];
    real_t *VpVol_f = new real_t[nr+1];    
    for(len_t ir=0; ir<nr;  ir++)
        VpVol[ir]   = EvaluateFluxSurfaceIntegral(ir, FLUXGRIDTYPE_DISTRIBUTION, unityFunc);
    for(len_t ir=0; ir<=nr; ir++)
        VpVol_f[ir] = EvaluateFluxSurfaceIntegral(ir, FLUXGRIDTYPE_RADIAL, unityFunc);
    
    rGrid->SetVpVol(VpVol,VpVol_f);
}


/**
 *  Evaluates the flux surface average <F> of a function F = F(B/Bmin, R/R0, |nabla r|^2) on radial grid point ir. 
 */
real_t FluxSurfaceAverager::CalculateFluxSurfaceAverage(len_t ir, fluxGridType fluxGridType, std::function<real_t(real_t,real_t,real_t)> F){
    real_t VpVol = GetVpVol(ir,fluxGridType);

    // treat singular point r=0 separately where orbit parameters are constant 
    if(VpVol == 0) 
        return F(1,1,1);
    // otherwise use regular method

    return EvaluateFluxSurfaceIntegral(ir,fluxGridType, F) / VpVol;
}


/**
 * The function returns the integrand of the flux surface integral at theta,
 * and is used with the adaptive quadrature
 */
struct FluxSurfaceIntegralParams {
    std::function<real_t(real_t,real_t,real_t)> Function; len_t ir; real_t Bmin;
    FluxSurfaceQuantity *B; FluxSurfaceQuantity *Jacobian; FluxSurfaceQuantity *ROverR0; 
    FluxSurfaceQuantity *NablaR2; fluxGridType fgType;
};
real_t FluxSurfaceAverager::FluxSurfaceIntegralFunction(real_t theta, void *p){
    struct FluxSurfaceIntegralParams *params = (struct FluxSurfaceIntegralParams *) p;
    len_t ir = params->ir;
    fluxGridType fluxGridType = params->fgType;
    real_t B = params->B->evaluateAtTheta(ir, theta, fluxGridType);
    real_t Jacobian = params->Jacobian->evaluateAtTheta(ir, theta, fluxGridType);
    real_t ROverR0 = params->ROverR0->evaluateAtTheta(ir, theta, fluxGridType);
    real_t NablaR2 = params->NablaR2->evaluateAtTheta(ir, theta, fluxGridType);
    real_t Bmin = params->Bmin;
    std::function<real_t(real_t,real_t,real_t)> F = params->Function;

    real_t BOverBmin;
    if(B==Bmin) // handles Bmin = 0 case
        BOverBmin=1;
    else
        BOverBmin = B/Bmin;
    
    return 2*M_PI*Jacobian*F(BOverBmin, ROverR0, NablaR2);
}


/**
 * Core function of this class: evaluates the flux surface integral 
 *    FluxSurfaceIntegral(X) = \int J*X dphi dtheta
 * taken over toroidal and poloidal angle, weighted by the 
 * spatial jacobian J. See doc/notes/theory section on 
 * Flux surface averages for further details.
 */
real_t FluxSurfaceAverager::EvaluateFluxSurfaceIntegral(len_t ir, fluxGridType fluxGridType, std::function<real_t(real_t,real_t,real_t)> F){
    real_t fluxSurfaceIntegral = 0;
    real_t Bmin = GetBmin(ir,fluxGridType);
    real_t Bmax = GetBmax(ir,fluxGridType);
    bool BminEqBmax = (Bmin==Bmax);

    // Calculate using fixed quadrature:
    if(!integrateAdaptive){
        const real_t *B = this->B->GetData(ir, fluxGridType);
        const real_t *Jacobian = this->Jacobian->GetData(ir,fluxGridType);
        const real_t *ROverR0 = this->ROverR0->GetData(ir, fluxGridType);
        const real_t *NablaR2 = this->NablaR2->GetData(ir, fluxGridType);
    
        for (len_t it = 0; it<ntheta_interp; it++){
                real_t BOverBmin;
                if(BminEqBmax)
                    BOverBmin=1;
                else
                    BOverBmin = B[it]/Bmin;

            fluxSurfaceIntegral += 2*M_PI*weights[it] * Jacobian[it] 
                * F(BOverBmin, ROverR0[it], NablaR2[it]);
        }
    // or by using adaptive quadrature:
    } else {
        gsl_function GSL_func; 
        FluxSurfaceIntegralParams params = {F, ir, Bmin, B, Jacobian, ROverR0, NablaR2, fluxGridType}; 
        GSL_func.function = &(FluxSurfaceIntegralFunction);
        GSL_func.params = &params;
        real_t epsabs = 0, epsrel = 1e-4, lim = gsl_adaptive->limit, error;
        gsl_integration_qag(&GSL_func, 0, theta_max,epsabs,epsrel,lim,QAG_KEY,gsl_adaptive,&fluxSurfaceIntegral, &error);
    }
    return fluxSurfaceIntegral;  
} 



/**
 * Deallocate quadrature.
 */
void FluxSurfaceAverager::DeallocateQuadrature(){
    gsl_integration_workspace_free(gsl_adaptive);
    gsl_integration_qaws_table_free(qaws_table_passing);
    gsl_integration_qaws_table_free(qaws_table_trapped);
    if(gsl_w != nullptr)
        gsl_integration_fixed_free(gsl_w);
}

/**
 * Sets quantities related to the quadrature rule used for 
 * evaluating the flux surface integrals. We use a quadrature
 *      integral( w(x, xmin, xmax)*F(x), xmin, xmax )
 *          = sum_i w_i F(x_i)
 * where theta is x_i, weights is w_i and quadratureWeightFunction 
 * is w(x, xmin, xmax).  ntheta_interp is the number of points i 
 * in the sum. The same quadrature is used on all radii.
 * Refer to GSL integration documentation for possible quadrature rules 
 * and corresponding weight functions:
 *      https://www.gnu.org/software/gsl/doc/html/integration.html
 */
void FluxSurfaceAverager::InitializeQuadrature(quadrature_method q_method){
    gsl_adaptive = gsl_integration_workspace_alloc(1000);
    std::function<real_t(real_t,real_t,real_t)>  QuadWeightFunction;
    if(geometryIsSymmetric)
        theta_max = M_PI;
    else 
        theta_max = 2*M_PI;

    const gsl_integration_fixed_type *quadratureRule = nullptr;
    switch(q_method){
        case QUAD_FIXED_LEGENDRE:
            // Legendre quadrature is often best for smooth finite functions on a finite interval
            quadratureRule = gsl_integration_fixed_legendre;
            QuadWeightFunction = [](real_t /*x*/, real_t /*x_min*/, real_t /*x_max*/)
                                    {return 1;};
            break;
        case QUAD_FIXED_CHEBYSHEV:
            // Chebyshev quadrature may be better for integration along trapped orbits
            // where the metric takes the form of this weight function.
            quadratureRule = gsl_integration_fixed_chebyshev;
            QuadWeightFunction = [](real_t x, real_t x_min, real_t x_max)
                                    {return 1/sqrt((x_max-x)*(x-x_min) );};
            break;
        case QUAD_ADAPTIVE:
            integrateAdaptive = true;
            return;
        default:
            throw FVMException("Quadrature rule '%d' not supported by FluxSurfaceAverager.", q_method);            
    }

    
    gsl_w = gsl_integration_fixed_alloc(quadratureRule,ntheta_interp,0,theta_max,0,0);
    this->theta = gsl_w->x;
    this->weights = gsl_w->weights;

    // We divide the weights by the QuadWeightFunction to cast the
    // equation on the form required by the quadrature rule.
    for(len_t it=0; it<ntheta_interp; it++)
        weights[it]/= QuadWeightFunction(theta[it],0,theta_max);

    // If symmetric field we integrate from 0 to pi and multiply result by 2. 
    if(geometryIsSymmetric)
        for(len_t it=0; it<ntheta_interp; it++)
            weights[it] *= 2;
}


/**
 * Sets reference magnetic field quantities that have been
 * generated by a RadialGridGenerator (FluxSurfaceAverager
 * takes ownership of these and will deallocate them).
 */
void FluxSurfaceAverager::SetReferenceMagneticFieldData(
    len_t ntheta_ref, real_t *theta_ref,
    real_t **B_ref, real_t **B_ref_f,
    real_t **Jacobian_ref, real_t **Jacobian_ref_f,
    real_t **ROverR0_ref ,real_t **ROverR0_ref_f, 
    real_t **NablaR2_ref, real_t **NablaR2_ref_f,
    real_t *theta_Bmin, real_t *theta_Bmin_f,
    real_t *theta_Bmax, real_t *theta_Bmax_f
){
    B->Initialize(B_ref,B_ref_f, theta_ref, ntheta_ref);
    Jacobian->Initialize(Jacobian_ref, Jacobian_ref_f, theta_ref, ntheta_ref);
    ROverR0->Initialize( ROverR0_ref,  ROverR0_ref_f,  theta_ref, ntheta_ref);
    NablaR2->Initialize( NablaR2_ref,  NablaR2_ref_f,  theta_ref, ntheta_ref);

    InitializeReferenceData(theta_Bmin, theta_Bmin_f, theta_Bmax, theta_Bmax_f);    
}

void FluxSurfaceAverager::InitializeReferenceData(
    real_t *theta_Bmin, real_t *theta_Bmin_f,
    real_t *theta_Bmax, real_t *theta_Bmax_f
){
    DeallocateReferenceData();
    this->theta_Bmin = theta_Bmin;
    this->theta_Bmin_f = theta_Bmin_f;
    this->theta_Bmax = theta_Bmax;
    this->theta_Bmax_f = theta_Bmax_f;    
}

void FluxSurfaceAverager::DeallocateReferenceData(){
    if(theta_Bmin==nullptr)
        return;

    delete [] theta_Bmin;
    delete [] theta_Bmin_f;
    delete [] theta_Bmax;
    delete [] theta_Bmax_f;
    
}


/**
 * Helper function to get Bmin from RadialGrid.
 */
real_t FluxSurfaceAverager::GetBmin(len_t ir,fluxGridType fluxGridType, real_t *theta_Bmin){
    if (fluxGridType == FLUXGRIDTYPE_RADIAL){
        if(theta_Bmin != nullptr)
            *theta_Bmin = this->theta_Bmin_f[ir];
        return rGrid->GetBmin_f(ir);
    } else {
        if(theta_Bmin != nullptr)
            *theta_Bmin = this->theta_Bmin[ir];
        return rGrid->GetBmin(ir);
    }
}
/**
 * Helper function to get Bmax from RadialGrid.
 */
real_t FluxSurfaceAverager::GetBmax(len_t ir,fluxGridType fluxGridType, real_t *theta_Bmax){
    if (fluxGridType == FLUXGRIDTYPE_RADIAL){
        if(theta_Bmax != nullptr)
            *theta_Bmax = this->theta_Bmax_f[ir];
        return rGrid->GetBmax_f(ir);
    } else {
        if(theta_Bmax != nullptr)
            *theta_Bmax = this->theta_Bmax[ir];
        return rGrid->GetBmax(ir);
    }
}
/**
 * Helper function to get VpVol from RadialGrid.
 */
real_t FluxSurfaceAverager::GetVpVol(len_t ir,fluxGridType fluxGridType){
    if (fluxGridType == FLUXGRIDTYPE_RADIAL){
        return rGrid->GetVpVol_f(ir);
    } else {
        return rGrid->GetVpVol(ir);
    }
}


/********************************************************************
 * The below functions evaluate bounce averages in p-xi coordinates *
 * at arbitrary (p, xi0) independently of MomentumGrids.            *
 ********************************************************************/

// Function that returns the bounce-integral integrand
struct generalBounceIntegralParams {
    len_t ir; real_t xi0; real_t p; real_t theta_b1; real_t theta_b2; fluxGridType fgType; 
    real_t Bmin; std::function<real_t(real_t,real_t,real_t,real_t)> F_eff; 
    FluxSurfaceAverager *fsAvg;};
real_t generalBounceIntegralFunc(real_t theta, void *par){
    struct generalBounceIntegralParams *params = (struct generalBounceIntegralParams *) par;
    
    len_t ir = params->ir;
    real_t xi0 = params->xi0;
    real_t p = params->p;
    real_t theta_b1 = params->theta_b1;
    real_t theta_b2 = params->theta_b2;
    fluxGridType fluxGridType = params->fgType;
    real_t Bmin = params->Bmin;
    FluxSurfaceAverager *fluxAvg = params->fsAvg; 
    std::function<real_t(real_t,real_t,real_t,real_t)> F_eff = params->F_eff;
    real_t B = fluxAvg->GetB()->evaluateAtTheta(ir,theta,fluxGridType); 
    real_t Jacobian = fluxAvg->GetJacobian()->evaluateAtTheta(ir,theta,fluxGridType);
    real_t ROverR0 = fluxAvg->GetROverR0()->evaluateAtTheta(ir,theta,fluxGridType);
    real_t NablaR2 = fluxAvg->GetNablaR2()->evaluateAtTheta(ir,theta,fluxGridType);
    real_t sqrtG = MomentumGrid::evaluatePXiMetricOverP2(p,xi0,B,Bmin);
    real_t xi0Sq = xi0*xi0;
    real_t BOverBmin, xiOverXi0;
    if(B==Bmin){ // for the Bmin=0 case
        BOverBmin = 1;
        xiOverXi0 = 1;
    } else { 
        BOverBmin = B/Bmin;
        real_t xiSq = (1-BOverBmin*(1-xi0Sq));
        if(xiSq < 0)
            return 0;
        xiOverXi0 = sqrt(xiSq/xi0Sq);
    }
    real_t F = F_eff(xiOverXi0,BOverBmin,ROverR0,NablaR2);
    real_t S = 2*M_PI*Jacobian*sqrtG*F;
    if((theta_b2-theta_b1) == 2*M_PI || F_eff(0,1,1,1)==0) 
        return S;
    else 
        return S*sqrt((theta-theta_b1)*(theta_b2-theta));
}

/**
 * Evaluates the bounce integral of the function F = F(xi/xi0, B/Bmin, ROverR0, NablaR2)  
 * at radial grid point ir, momentum p and pitch xi0, using an adaptive quadrature.
 */
real_t FluxSurfaceAverager::EvaluatePXiBounceIntegralAtP(len_t ir, real_t p, real_t xi0, fluxGridType fluxGridType, std::function<real_t(real_t,real_t,real_t,real_t)> F){
    real_t theta_Bmin, theta_Bmax;
    real_t Bmin = GetBmin(ir,fluxGridType, &theta_Bmin);
    real_t Bmax = GetBmax(ir,fluxGridType, &theta_Bmax);
    real_t BminOverBmax;
    if(Bmin==Bmax) // handles Bmax=0 case
        BminOverBmax = 1; 
    else
        BminOverBmax = Bmin/Bmax;        

    std::function<real_t(real_t,real_t,real_t,real_t)> F_eff;
    bool isTrapped = ( (1-xi0*xi0) > BminOverBmax);
    gsl_integration_qaws_table *qaws_table;
    real_t theta_b1, theta_b2;
    // If trapped, adds contribution from -xi0
    if (isTrapped){
        // negative-pitch particles do not exist independently; are described by the positive pitch counterpart
        if(xi0<0)
            return 0;
        F_eff = [&](real_t x, real_t  y, real_t z, real_t w){return  F(x,y,z,w) + F(-x,y,z,w) ;};
        FindBouncePoints(ir, Bmin, theta_Bmin, theta_Bmax, this->B, xi0, fluxGridType, &theta_b1, &theta_b2,gsl_fsolver);
        if(theta_b1==theta_b2)
            return 0;
//        real_t h = theta_b2-theta_b1;
//        theta_b1 += 0*1e-1*h;
//        theta_b2 -= 0*1e-1*h;
        if(F_eff(0,1,1,1)!=0)
            qaws_table = qaws_table_trapped;
        else
            qaws_table = qaws_table_passing;
    } else { 
        F_eff = F;
        theta_b1 = 0;
        theta_b2 = 2*M_PI;
        qaws_table = qaws_table_passing;
    }

    gsl_function GSL_func;
    GSL_func.function = &(generalBounceIntegralFunc);
    generalBounceIntegralParams params = {ir,xi0,p,theta_b1,theta_b2,fluxGridType,Bmin,F_eff,this};
    GSL_func.params = &params;
    real_t bounceIntegral, error; 

    real_t epsabs = 0, epsrel = 5e-4, lim = gsl_adaptive->limit; 
    if(qaws_table==qaws_table_trapped)
        gsl_integration_qaws(&GSL_func,theta_b1,theta_b2,qaws_table,epsabs,epsrel,lim,gsl_adaptive,&bounceIntegral,&error);
    else
        gsl_integration_qag(&GSL_func,theta_b1,theta_b2,epsabs,epsrel,lim,QAG_KEY,gsl_adaptive,&bounceIntegral,&error);
    
    return bounceIntegral;
}

// Evaluates the bounce average {F} of a function F = F(xi/xi0, B/Bmin) on grid point (ir,i,j). 
real_t FluxSurfaceAverager::CalculatePXiBounceAverageAtP(len_t ir, real_t p, real_t xi0, fluxGridType fluxGridType, std::function<real_t(real_t,real_t,real_t,real_t)> F){
    
    std::function<real_t(real_t,real_t,real_t,real_t)> FUnity = [](real_t,real_t,real_t,real_t){return 1;};
    real_t Vp = EvaluatePXiBounceIntegralAtP(ir, p, xi0, fluxGridType, FUnity);    
    /**
     * Treat special case: 
     * Either r=0 (particle doesn't move in the poloidal plane) or 
     * xi0=0 in inhomogeneous magnetic field (infinitely deeply trapped on low-field side).
     * TODO: consider more carefully what happens with xi/xi0 in the xi0=0 case.
     */
    if(Vp == 0)  
        return F(1,1,1,1);

    return EvaluatePXiBounceIntegralAtP(ir, p, xi0, fluxGridType, F) / Vp;
}


/**
 * Returns xi(xi0)^2 and is used in the determination of the bounce points. 
 */
struct xiFuncParams {real_t xi0; len_t ir; real_t Bmin; const FluxSurfaceQuantity *B; fluxGridType fgType; };
real_t FluxSurfaceAverager::xiParticleFunction(real_t theta, void *p){
    struct xiFuncParams *params = (struct xiFuncParams *) p;
    
    real_t xi0 = params->xi0; 
    len_t ir   = params->ir;
    fluxGridType fluxGridType = params->fgType;
    real_t Bmin = params->Bmin;
    const FluxSurfaceQuantity *B = params->B;
    return 1 - (1-xi0*xi0) *B->evaluateAtTheta(ir,theta,fluxGridType) / Bmin;
}

/**
 * Returns poloidal bounce points theta_bounce1 and theta_bounce2 with a root finding algorithm
 */
void FluxSurfaceAverager::FindBouncePoints(len_t ir, real_t Bmin, real_t theta_Bmin, real_t theta_Bmax, const FluxSurfaceQuantity *B, real_t xi0, fluxGridType fluxGridType,  real_t *theta_b1, real_t *theta_b2, gsl_root_fsolver* gsl_fsolver){
    // define GSL function xi_particle as function of theta which evaluates B_interpolator(_fr) at theta.
    xiFuncParams xi_params = {xi0,ir,Bmin,B,fluxGridType}; 
    gsl_function gsl_func;
    gsl_func.function = &(xiParticleFunction);
    gsl_func.params = &xi_params;

    // A guess can be estimated assuming that B ~ 1/[R+rcos(theta)] 
    // -- probably often a good guess. Then:
    // real_t cos_theta_b_guess = ((1-xi0*xi0)/Bmin - (1/Bmin + 1/Bmax)/2 ) * 2/( 1/Bmin - 1/Bmax );

    FluxSurfaceAverager::FindThetas(theta_Bmin, theta_Bmax, theta_b1, theta_b2, gsl_func, gsl_fsolver);
}



/**
 * Finds the theta that solves gsl_func = 0.
 * Takes a theta interval theta \in [x_lower, x_upper] and iterates at most 
 * max_iter times (or to a relative error of rel_error) to find an estimate 
 * for theta \in [x_lower, x_upper] 
 */
void FluxSurfaceAverager::FindRoot(real_t *x_lower, real_t *x_upper, real_t *root, gsl_function gsl_func, gsl_root_fsolver *gsl_fsolver){
    gsl_root_fsolver_set (gsl_fsolver, &gsl_func, *x_lower, *x_upper); // finds root in [0,pi] using GSL_rootsolver_type algorithm
    int status;
    real_t rel_error = 1e-6;
    len_t max_iter = 50;    
    for (len_t iteration = 0; iteration < max_iter; iteration++ ){
        status   = gsl_root_fsolver_iterate (gsl_fsolver);
        *root    = gsl_root_fsolver_root (gsl_fsolver);
        *x_lower = gsl_root_fsolver_x_lower (gsl_fsolver);
        *x_upper = gsl_root_fsolver_x_upper (gsl_fsolver);
        status   = gsl_root_test_interval (*x_lower, *x_upper,
                                            0, rel_error);

        if (status == GSL_SUCCESS){
            break;
        }
    }
}


/**
 * Finds the two poloidal angles theta1 and theta2 that solves 
 *  gsl_func.function(theta) = 0,
 * returning solutions that satisfy 
 *  gsl_function.function > 0. 
 * theta2 is assumed to exist on the interval [theta_Bmin, theta_Bmax], 
 * and theta1 on the interval [theta_Bmax-2*pi, theta_Bmin]
 */
void FluxSurfaceAverager::FindThetas(real_t theta_Bmin, real_t theta_Bmax, real_t *theta1, real_t *theta2, gsl_function gsl_func, gsl_root_fsolver *gsl_fsolver){
    real_t root=0;

    real_t x_lower = theta_Bmin;
    real_t x_upper = theta_Bmax;
    FindRoot(&x_lower, &x_upper, &root, gsl_func, gsl_fsolver);

    if( gsl_func.function(x_lower, gsl_func.params) >= 0 )
        *theta2 = x_lower;
    else if ( gsl_func.function(x_upper, gsl_func.params) >= 0 )
        *theta2 = x_upper;
    else
        throw FVMException("FluxSurfaceAverager: unable to find valid theta root.");

    x_lower = theta_Bmax - 2*M_PI;
    x_upper = theta_Bmin;
    FindRoot(&x_lower, &x_upper, &root, gsl_func, gsl_fsolver);

    if( gsl_func.function(x_lower, gsl_func.params) >= 0 )
        *theta1 = x_lower;
    else if( gsl_func.function(x_upper, gsl_func.params) >= 0 )
        *theta1 = x_upper;
    else
        throw FVMException("FluxSurfaceAverager: unable to find valid theta root.");

    
}


