/**
 * Implementation of a class which handles the calculation of the 
 * parallel diffusion coefficient nu_||. The D^pp component of the 
 * collision operator is defined as (m_e c)^2 nu_|| (and not the 
 * definition that one often encounters in the literature, where it 
 * has been multiplied by p^2/2).
 * It should be rebuilt after the SlowingDownFrequency has been rebuilt.
 */

/**
 * For the linearised case with a known temperature, it is uniquely prescribed by nu_s,
 * by requiring the preservation of the Maxwell-Jüttner steady-state distribution. 
 * The method is described in Appendix A.1 of doc/notes/theory.pdf.
 * The non-linear contribution corresponds to the isotropic component of the
 * non-relativistic operator following Rosenbluth, Macdonald & Judd, Phys Rev (1957),
 * and is described in doc/notes/theory.pdf Appendix B.
 */

// TODO: Implement real_t *GetUnknownPartialContribution(len_t id_unknown,len_t fluxGridMode, real_t *&partQty);

#include "DREAM/Equations/ParallelDiffusionFrequency.hpp"
#include "DREAM/NotImplementedException.hpp"

using namespace DREAM;


/**
 * Constructor.
 */
ParallelDiffusionFrequency::ParallelDiffusionFrequency(FVM::Grid *g, FVM::UnknownQuantityHandler *u, IonHandler *ih,  
                SlowingDownFrequency *nuS, CoulombLogarithm *lnLee,
                enum OptionConstants::momentumgrid_type mgtype,  struct collqty_settings *cqset)
                : CollisionQuantity(g,u,ih,mgtype,cqset){
    this->lnLambdaEE = lnLee;
    this->nuS = nuS;
    isSuperthermal = (cqset->collfreq_mode==OptionConstants::COLLQTY_COLLISION_FREQUENCY_MODE_SUPERTHERMAL);
}

/**
 * Destructor.
 */
ParallelDiffusionFrequency::~ParallelDiffusionFrequency(){
    DeallocateCollisionQuantities();    
    DeallocatePartialQuantities();      
}

/**
 * Calculates the parallel diffusion frequency from the values of the slowing-down frequency.
 */
void ParallelDiffusionFrequency::AssembleQuantity(real_t **&collisionQuantity, len_t nr, len_t np1, len_t np2, FVM::fluxGridType fluxGridType){
    if(isSuperthermal){
        for(len_t ir=0;ir<nr;ir++)
            for(len_t it=0;it<np1*np2;it++)
                collisionQuantity[ir][it] = 0;
        return;
    }
    real_t *const* nuSQty;
    const real_t *gammaVec;
    if(fluxGridType == FVM::FLUXGRIDTYPE_DISRIBUTION){
        nuSQty = nuS->GetValue();
        gammaVec = mg->GetGamma();
    } else if(fluxGridType == FVM::FLUXGRIDTYPE_RADIAL){
        nuSQty = nuS->GetValue_fr();
        gammaVec = mg->GetGamma();
    }else if(fluxGridType == FVM::FLUXGRIDTYPE_P1){
        nuSQty = nuS->GetValue_f1();
        gammaVec = mg->GetGamma_f1();
    }else if(fluxGridType == FVM::FLUXGRIDTYPE_P2){
        nuSQty = nuS->GetValue_f2();
        gammaVec = mg->GetGamma_f2();
    }
    len_t pind;
    for(len_t ir=0; ir<nr; ir++){
        for(len_t i=0; i<np1; i++){
            for(len_t j=0; j<np2; j++){
                pind = np1*j+i;
                collisionQuantity[ir][pind] = nuSQty[ir][pind] * rescaleFactor(ir,gammaVec[pind]);
            }
        }
    }

}

void ParallelDiffusionFrequency::AllocatePartialQuantities(){
    DeallocatePartialQuantities();
    Tnormalized = new real_t[nr];

    if (isNonlinear){
        nonlinearMat = new real_t*[np1+1]; 
        for (len_t i = 0; i<np1+1; i++){
            nonlinearMat[i] = new real_t[np1];
        }
        const real_t *p = mg->GetP1();
        trapzWeights = new real_t[np1];
        for (len_t i = 1; i<np1-1; i++){
            trapzWeights[i] = (p[i+1]-p[i-1])/2;
        }
    }

}
void ParallelDiffusionFrequency::DeallocatePartialQuantities(){
    if(Tnormalized !=nullptr)
        delete [] Tnormalized;

    if(nonlinearMat != nullptr){
        for(len_t i = 0; i<np1+1;i++){
            delete [] nonlinearMat[i];
        }
        delete [] nonlinearMat;
        delete [] trapzWeights;
    }

}

/**
 * Rebuilds the part of the calculation that depends on plasma parameters.
 */
void ParallelDiffusionFrequency::RebuildPlasmaDependentTerms(){
    if(isSuperthermal)
        return;
    real_t *Tcold = unknowns->GetUnknownData(id_Tcold);
    for(len_t ir=0; ir<nr;ir++){
        Tnormalized[ir] = Tcold[ir]/Constants::mc2inEV;
    }
}

/**
 * The factor by which the slowing-down frequency should be multiplied in order
 * to yield the parallel diffusion frequency.
 */
real_t ParallelDiffusionFrequency::rescaleFactor(len_t ir, real_t gamma){
    return Tnormalized[ir]*gamma;
}

/**
 * Evaluates the frequency at radial grid point ir and momentum p.
 */
real_t ParallelDiffusionFrequency::evaluateAtP(len_t ir, real_t p){
    if(isSuperthermal)
        return 0;
    return rescaleFactor(ir,sqrt(1+p*p))*nuS->evaluateAtP(ir,p);
}

real_t ParallelDiffusionFrequency::evaluateAtP(len_t ir, real_t p, OptionConstants::collqty_collfreq_type collfreq_type, OptionConstants::collqty_collfreq_mode collfreq_mode){
    if(isSuperthermal)
        return 0;
    return rescaleFactor(ir,sqrt(1+p*p))*nuS->evaluateAtP(ir,p,collfreq_type,collfreq_mode);
}



/**
 * Adds the non-linear contribution to the parallel diffusion frequency.
 */
void ParallelDiffusionFrequency::AddNonlinearContribution(){
    real_t *fHot = unknowns->GetUnknownData(id_fhot);
    real_t *fHotContribution = new real_t[nr*np1*(np1+1)];
    GetNonlinearPartialContribution(lnLambdaEE->GetLnLambdaC(),fHotContribution);

    for (len_t ir=0;ir<nr;ir++)
        for(len_t i=0; i<np1+1; i++)
            for(len_t ip=0; ip<np1; ip++)
                collisionQuantity_f1[ir][i] += fHotContribution[ip*(np1+1)*nr + ir*(np1+1) + i] * fHot[np1*ir+ip];
}

void ParallelDiffusionFrequency::GetNonlinearPartialContribution(const real_t* lnLc, real_t *&partQty){
    if(partQty==nullptr){
        partQty = new real_t[np1*(np1+1)*nr];
    }

    for(len_t it=0; it < np1*(np1+1)*nr; it++){
        partQty[it] = 0;
    }

    for(len_t i=0; i<np1+1; i++)
        for(len_t ir=0;ir<nr;ir++)
            for(len_t ip=0; ip<np1; ip++)
                partQty[ip*(np1+1)*nr + (np1+1)*ir + i] = lnLc[ir]*nonlinearMat[i][ip];
}


/**
 * Rebuilds partial contributions that only depend on the grid.
 */
void ParallelDiffusionFrequency::RebuildConstantTerms(){
    if(isNonlinear)
        calculateIsotropicNonlinearOperatorMatrix();
}

/**
 *  Calculates a Rosenbluth potential matrix defined such that when it is muliplied
 * by the f_hot distribution vector, yields the parallel diffusion frequency.
 */
void ParallelDiffusionFrequency::calculateIsotropicNonlinearOperatorMatrix(){

    if( !(isPXiGrid && (mg->GetNp2() == 1)) )
        throw NotImplementedException("Nonlinear collisions only implemented for hot tails (np2=1) and p-xi grid");

    
    const real_t *p_f = mg->GetP1_f();
    const real_t *p = mg->GetP1();

    // See doc/notes/theory.pdf appendix B for details on discretization of integrals;
    // uses a trapezoidal rule
    real_t p2, p2f;
    real_t weightsIm1, weightsI;
    for (len_t i = 1; i<np1+1; i++){
        p2f = p_f[i]*p_f[i];
        p2 = p[0]*p[0];
        nonlinearMat[i][0]   = (4*M_PI/3) * constPreFactor*( (p[1]-p[0])/2  + p[0]/5 )* p2*p2/(p_f[i]*p2f);
        for (len_t ip = 1; ip < i-1; ip++){
            p2 = p[ip]*p[ip];
            nonlinearMat[i][ip]   = (4*M_PI/3) * constPreFactor* trapzWeights[ip]*p2*p2 / (p_f[i]*p2f);
        } 
        p2 = p[i-1]*p[i-1];
        weightsIm1 = (p[i-1]-p[i-2])/2 + (p_f[i]-p[i-1])/(p[i]-p[i-1])*( (2*p[i]-p_f[i]-p[i-1])/2 );
        nonlinearMat[i][i-1]   = (4*M_PI/3) * constPreFactor * weightsIm1*p2*p2 / (p_f[i]*p2f);
        p2 = p[i]*p[i];
        weightsI = (p_f[i]-p[i-1])*(p_f[i]-p[i-1])/(p[i]-p[i-1]);
        nonlinearMat[i][i]   = (4*M_PI/3) * constPreFactor * weightsI*p2*p2 / (p_f[i]*p2f);

        // add contribution from p'>p terms near p'=p
        p2 = p[i-1]*p[i-1];
        weightsIm1 = (1.0/2)*(p[i]-p_f[i])*(p[i]-p_f[i])/(p[i]-p[i-1]);
        nonlinearMat[i][i-1]   += (4*M_PI/3) * constPreFactor * weightsIm1*p[i-1];
        p2 = p[i]*p[i];
        weightsI = (p[i+1]-p[i])/2 + (1.0/2)*(p[i]-p_f[i])*(p_f[i]+p[i]-2*p[i-1])/(p[i]-p[i-1]);
        nonlinearMat[i][i]   += (4*M_PI/3) * constPreFactor * weightsI * p[i];


        for (len_t ip = i+1; ip < np1-1; ip++){
            nonlinearMat[i][ip] = (4*M_PI/3) * constPreFactor* trapzWeights[ip]*p[ip];
        } 
        real_t weightsEnd = (p[np1-1]-p[np1-2])/2;
        nonlinearMat[i][np1-1] = (4*M_PI/3) * constPreFactor* weightsEnd*p[np1-1];
        
    }


}