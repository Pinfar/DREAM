/**
 * Implementation of a class that calculates and stores quantities related to the runaway growth rate, 
 * e.g. the effective critical field and runaway momentum, as well as avalanche and Dreicer growths etc.
 */

#include "DREAM/Equations/RunawayFluid.hpp"
#include "DREAM/NotImplementedException.hpp"

using namespace DREAM;

const real_t RunawayFluid::tritiumHalfLife =  3.888e8;    // 12.32 years, in seconds
const real_t RunawayFluid::tritiumDecayEnergyEV = 18.6e3; // maximum beta electron kinetic energy in eV 


const len_t  RunawayFluid::conductivityLenT = 14;
const len_t  RunawayFluid::conductivityLenZ = 6;
const real_t RunawayFluid::conductivityBraams[conductivityLenZ*conductivityLenT] = {3.75994, 3.7549, 3.7492, 3.72852, 3.6842, 3.57129, 3.18206, 2.65006, 2.03127, 1.33009, 0.94648, 0.67042, 0.42422, 0.29999, 7.42898, 7.27359, 7.12772, 6.73805, 6.20946, 5.43667, 4.13733, 3.13472, 2.27862, 1.45375, 1.02875, 0.72743, 0.46003, 0.32528, 8.7546, 8.53281, 8.32655, 7.78445, 7.06892, 6.06243, 4.47244, 3.32611, 2.39205, 1.51805, 1.07308, 0.75853, 0.47965, 0.33915, 10.39122, 10.07781, 9.78962, 9.04621, 8.09361, 6.80431, 4.8805, 3.57303, 2.54842, 1.61157, 1.13856, 0.80472, 0.50885, 0.35979, 11.33006, 10.95869, 10.61952, 9.75405, 8.66306, 7.21564, 5.11377, 3.72206, 2.64827, 1.67382, 1.18263, 0.83593, 0.52861, 0.37377, 12.76615, 12.29716, 11.87371, 10.81201, 9.50746, 7.82693, 5.47602, 3.96944, 2.82473, 1.7887, 1.2649, 0.89443, 0.56569, 0.4};
const real_t RunawayFluid::conductivityTmc2[conductivityLenT] = {0,0.01,0.02,0.05,0.1,0.2,0.5,1,2,5,10,20,50,100};
const real_t RunawayFluid::conductivityX[conductivityLenZ]    = {0,0.090909090909091,0.166666666666667,0.333333333333333,0.5,1};

/**
 * Constructor.
 */
RunawayFluid::RunawayFluid(FVM::Grid *g, FVM::UnknownQuantityHandler *u, SlowingDownFrequency *nuS, 
    PitchScatterFrequency *nuD, CoulombLogarithm *lnLee, CoulombLogarithm *lnLei, CollisionQuantity::collqty_settings *cqs){
    this->gridRebuilt = true;
    this->rGrid = g->GetRadialGrid();
    this->nuS = nuS;
    this->nuD = nuD;
    this->lnLambdaEE = lnLee;
    this->lnLambdaEI = lnLei;
    this->collQtySettings = cqs;
    this->unknowns = u;
    id_ncold = this->unknowns->GetUnknownID(OptionConstants::UQTY_N_COLD);
    id_ntot  = this->unknowns->GetUnknownID(OptionConstants::UQTY_N_TOT);
    id_ni    = this->unknowns->GetUnknownID(OptionConstants::UQTY_ION_SPECIES);
    id_Tcold = this->unknowns->GetUnknownID(OptionConstants::UQTY_T_COLD);
    id_Eterm = this->unknowns->GetUnknownID(OptionConstants::UQTY_E_FIELD);

    const gsl_root_fsolver_type *GSL_rootsolver_type = gsl_root_fsolver_brent;
    const gsl_min_fminimizer_type *fmin_type = gsl_min_fminimizer_brent;
    this->gsl_ad_w = gsl_integration_workspace_alloc(1000);
    this->fsolve = gsl_root_fsolver_alloc (GSL_rootsolver_type);
    this->fmin = gsl_min_fminimizer_alloc(fmin_type);

    collSettingsForEc = new CollisionQuantity::collqty_settings;
    // Set collision settings for the Eceff calculation; always include bremsstrahlung and energy-dependent 
    // Coulomb logarithm, and also use superthermal mode (which avoids weird solutions near p ~ p_Te). 
    // The user only chooses between completely screened, non-screened or partially screened.
    collSettingsForEc->collfreq_type = collQtySettings->collfreq_type;
    collSettingsForEc->lnL_type = OptionConstants::COLLQTY_LNLAMBDA_ENERGY_DEPENDENT;
    collSettingsForEc->collfreq_mode = OptionConstants::COLLQTY_COLLISION_FREQUENCY_MODE_SUPERTHERMAL;
    collSettingsForEc->bremsstrahlung_mode = OptionConstants::EQTERM_BREMSSTRAHLUNG_MODE_STOPPING_POWER;

    // Set collision settings for the critical-momentum calculation: takes input settings but 
    // enforces superthermal mode which can cause unwanted thermal solutions to pc.
    collSettingsForPc = new CollisionQuantity::collqty_settings;
    collSettingsForPc->collfreq_type = collQtySettings->collfreq_type;
    collSettingsForPc->lnL_type      = collQtySettings->lnL_type;
    collSettingsForPc->bremsstrahlung_mode = collQtySettings->bremsstrahlung_mode;
    collSettingsForPc->collfreq_mode = OptionConstants::COLLQTY_COLLISION_FREQUENCY_MODE_SUPERTHERMAL;

    const gsl_interp2d_type *gsl_T = gsl_interp2d_bilinear; 
    gsl_cond = gsl_interp2d_alloc(gsl_T, conductivityLenT,conductivityLenZ);
    gsl_xacc = gsl_interp_accel_alloc();
    gsl_yacc = gsl_interp_accel_alloc();

    gsl_interp2d_init(gsl_cond, conductivityTmc2, conductivityX, conductivityBraams,conductivityLenT,conductivityLenZ);

}

/**
 * Destructor.
 */
RunawayFluid::~RunawayFluid(){
    DeallocateQuantities();

    gsl_integration_workspace_free(gsl_ad_w);
    gsl_root_fsolver_free(fsolve);
    gsl_min_fminimizer_free(fmin);

    gsl_interp2d_free(gsl_cond);
    gsl_interp_accel_free(gsl_xacc);
    gsl_interp_accel_free(gsl_yacc);

    delete [] collSettingsForEc;
    delete [] collSettingsForPc;
}

/**
 * Rebuilds all runaway quantities if plasma parameters have changed.
 */
void RunawayFluid::Rebuild(bool useApproximateMethod){
    if(!parametersHaveChanged())
        return;
        
    if(gridRebuilt){
        nr = rGrid->GetNr();
        AllocateQuantities();
        gridRebuilt = false;
    }
    ncold   = unknowns->GetUnknownData(id_ncold);
    ntot    = unknowns->GetUnknownData(id_ntot);
    Tcold   = unknowns->GetUnknownData(id_Tcold);
    Eterm   = unknowns->GetUnknownData(id_Eterm);
    
    // The collision frequencies (nuS, nuD) uses the coulomb logarithms 
    // in a way that requires them to be rebuilt here.
    lnLambdaEE->RebuildRadialTerms();
    lnLambdaEI->RebuildRadialTerms();
    nuS->RebuildRadialTerms();
    nuD->RebuildRadialTerms();

    CalculateDerivedQuantities();
    CalculateEffectiveCriticalField(useApproximateMethod);
    CalculateCriticalMomentum();
    CalculateGrowthRates();
}

/** 
 * Returns true if any unknown quantities that affect runaway rates have changed. 
 */
bool RunawayFluid::parametersHaveChanged(){
    return unknowns->HasChanged(id_ncold) || unknowns->HasChanged(id_Tcold) || unknowns->HasChanged(id_ni) || 
            unknowns->HasChanged(id_Eterm) || gridRebuilt;
}


/**
 * Calculates the Connor-Hastie field Ec using the relativistic lnLambda
 * and the Dreicer field ED using the thermal lnLambda.
 */
void RunawayFluid::CalculateDerivedQuantities(){
    real_t *T_cold = unknowns->GetUnknownData(id_Tcold);
    for (len_t ir=0; ir<nr; ir++){
        Ec_free[ir] = lnLambdaEE->evaluateLnLambdaC(ir) * ncold[ir] * constPreFactor * Constants::me * Constants::c / Constants::ec;
        Ec_tot[ir]  = lnLambdaEE->evaluateLnLambdaC(ir) * ntot[ir]  * constPreFactor * Constants::me * Constants::c / Constants::ec;
        EDreic[ir]  = lnLambdaEE->evaluateLnLambdaT(ir) * ncold[ir] * constPreFactor * (Constants::me * Constants::c / Constants::ec) * (Constants::mc2inEV / T_cold[ir]);
        tauEERel[ir] = 1/(lnLambdaEE->evaluateLnLambdaC(ir) * ncold[ir] * constPreFactor); // = m*c/(e*Ec_free)
        tauEETh[ir]  = 1/(lnLambdaEE->evaluateLnLambdaT(ir) * ncold[ir] * constPreFactor) * pow(2*T_cold[ir]/Constants::mc2inEV,1.5); 
    }
}


/**
 * Is called to specify that the grid has been rebuilt, and that reallocation of all stored quantities is needed.
 */
void RunawayFluid::GridRebuilt(){
    gridRebuilt = true;
    lnLambdaEE->GridRebuilt();
    lnLambdaEI->GridRebuilt();
    nuS->GridRebuilt();
    nuD->GridRebuilt();    
}



/**
 * Finds the root of the provided gsl_function in the interval x_lower < root < x_upper. 
 * Is used both in the Eceff and pCrit calculations. 
 */
void RunawayFluid::FindRoot(real_t x_lower, real_t x_upper, real_t *root, gsl_function gsl_func, gsl_root_fsolver *s){
    
    gsl_root_fsolver_set (s, &gsl_func, x_lower, x_upper); 

    int status;
    real_t epsrel = 3e-3;
    len_t max_iter = 30;
    for (len_t iteration = 0; iteration < max_iter; iteration++ ){
        status   = gsl_root_fsolver_iterate (s);
        *root    = gsl_root_fsolver_root (s);
        x_lower = gsl_root_fsolver_x_lower (s);
        x_upper = gsl_root_fsolver_x_upper (s);
        status   = gsl_root_test_interval (x_lower, x_upper, 0, epsrel);

        if (status == GSL_SUCCESS){
            
            break;
        }
    }
}

/**
 * A (crude) method which expands the interval [x_lower, x_upper] 
 * if a root of the provided gsl_function is not in the interval.
 */
void RunawayFluid::FindInterval(real_t *x_lower, real_t *x_upper, gsl_function gsl_func ){
    bool isLoUnderestimate = (gsl_func.function(*x_lower, gsl_func.params ) > 0);
    bool isUpOverestimate = (gsl_func.function(*x_upper, gsl_func.params ) < 0);
    while(!isLoUnderestimate){
        *x_upper = *x_lower;
        *x_lower *= 0.7;
        isLoUnderestimate = (gsl_func.function(*x_lower, gsl_func.params ) > 0);
        isUpOverestimate = true;
    }
    while (!isUpOverestimate){
        *x_upper *= 1.4;
        isUpOverestimate = (gsl_func.function(*x_upper, gsl_func.params ) < 0);
    }
}


///////////////////////////////////////////////////////////////////
/////////// BEGINNING OF BLOCK WITH METHODS RELATED TO  ///////////
/////////// CALCULATION OF THE EFFECTIVE CRITICAL FIELD /////////// 
///////////////////////////////////////////////////////////////////

/**
 * Parameter struct which is passed to all GSL functions involved in the Eceff calculations.
 */
struct UContributionParams {FVM::RadialGrid *rGrid; RunawayFluid *rf; SlowingDownFrequency *nuS; PitchScatterFrequency *nuD; len_t ir; real_t p; FVM::fluxGridType fgType; 
                            real_t Eterm; std::function<real_t(real_t,real_t,real_t)> Func; gsl_integration_workspace *gsl_ad_w;
                            gsl_min_fminimizer *fmin;real_t p_ex_lo; real_t p_ex_up; bool useApproximateMethod; CollisionQuantity::collqty_settings *collSettingsForEc;};



#include "RunawayFluid.UFunc.cpp"

/**
 * Calculates and stores the effective critical field for runaway generation.
 * The calculation is based on Eq (21) in Hesslow et al, PPCF 60, 074010 (2018)
 * but has been generalized to account for inhomogeneous magnetic fields. The
 * method implemented here is outlined in DREAM/doc/notes/theory.
 * Essentially, the critical effective electric field is defined as the E
 * for which the maximum (with respect to p) of U(p) equals 0. Here, U
 * is the net momentum advection term averaged over an analytic pitch
 * angle distribution.
 */
void RunawayFluid::CalculateEffectiveCriticalField(bool useApproximateMethod){
    effectiveCriticalField = new real_t[nr];
    
    // placeholder quantities that will be overwritten by the GSL functions
    std::function<real_t(real_t,real_t,real_t)> Func = [](real_t,real_t,real_t){return 0;};
    real_t Eterm = 0, p = 0, p_ex_lo = 0, p_ex_up = 0;

    real_t ELo, EUp;
    UContributionParams params; 
    gsl_function UExtremumFunc;
    for (len_t ir=0; ir<this->nr; ir++){
        params = {rGrid, this, nuS,nuD, ir, p, FVM::FLUXGRIDTYPE_DISTRIBUTION, Eterm, Func, gsl_ad_w,
                            fmin, p_ex_lo, p_ex_up,useApproximateMethod,collSettingsForEc};
        UExtremumFunc.function = &(FindUExtremumAtE);
        UExtremumFunc.params = &params;

        /**
         * Initial guess: Eceff is between 0.9*Ec_tot and 1.5*Ec_tot
         */
        ELo = .9*Ec_tot[ir];
        EUp = 1.5*Ec_tot[ir];
        FindInterval(&ELo, &EUp, UExtremumFunc);
        FindRoot(ELo,EUp, &effectiveCriticalField[ir], UExtremumFunc,fsolve);
    }
}

/**
 *  Returns the minimum of -U (with respect to p) at a given Eterm 
 */
real_t RunawayFluid::FindUExtremumAtE(real_t Eterm, void *par){
    struct UContributionParams *params = (struct UContributionParams *) par;
    params->Eterm = Eterm;
    gsl_min_fminimizer *gsl_fmin = params->fmin;


    real_t p_ex_guess, p_ex_lo, p_ex_up;

    gsl_function F;
    F.function = &(UAtPFunc);
    F.params = params;
    real_t p_upper_threshold = 1000; // larger momenta are not physically relevant in our scenarios
    FindPExInterval(&p_ex_guess, &p_ex_lo, &p_ex_up, params, p_upper_threshold);

    // If the extremum is at a larger momentum than p_upper_threshold (or doesn't exist at all), 
    // we will define Eceff as the value where U(p_upper_threshold) = 0. 
    if(p_ex_up > p_upper_threshold)
        return UAtPFunc(p_upper_threshold,params);

    gsl_min_fminimizer_set(gsl_fmin, &F, p_ex_guess, p_ex_lo, p_ex_up);

    int status;
    real_t rel_error = 5e-2, abs_error=0;
    len_t max_iter = 30;
    for (len_t iteration = 0; iteration < max_iter; iteration++ ){
        status     = gsl_min_fminimizer_iterate(gsl_fmin);
        p_ex_guess = gsl_min_fminimizer_x_minimum(gsl_fmin);
        p_ex_lo    = gsl_min_fminimizer_x_lower(gsl_fmin);
        p_ex_up    = gsl_min_fminimizer_x_upper(gsl_fmin);
        status     = gsl_root_test_interval(p_ex_lo, p_ex_up, abs_error, rel_error);

        if (status == GSL_SUCCESS){
            break;
        }
    }

    real_t minimumFValue = gsl_min_fminimizer_f_minimum(gsl_fmin);
    return minimumFValue;
}


/**
 *  Finds an interval p \in [p_ex_lower, p_ex_upper] in which a minimum of -U(p) exists.  
 */
void RunawayFluid::FindPExInterval(real_t *p_ex_guess, real_t *p_ex_lower, real_t *p_ex_upper, void *par, real_t p_upper_threshold){
    struct UContributionParams *params = (struct UContributionParams *) par;

    *p_ex_lower = 1;
    *p_ex_upper = 100;
    *p_ex_guess = 10;
    real_t F_lo = UAtPFunc(*p_ex_lower,params);
    real_t F_up = UAtPFunc(*p_ex_upper,params);
    real_t F_g  = UAtPFunc(*p_ex_guess,params);
    
    if( (F_g < F_up) && (F_g < F_lo) ) // at least one minimum exists on the interval
        return;
    else if ( F_g > F_lo){ // Minimum located at p<p_ex_guess
        while(F_g > F_lo){
            *p_ex_upper = *p_ex_guess;
            *p_ex_guess = *p_ex_lower;
            *p_ex_lower /= 5;
            F_g = F_lo; //UAtPFunc(*p_ex_guess,params);
            F_lo = UAtPFunc(*p_ex_lower,params);
        }
    } else { // Minimum at p>p_ex_guss
        while( (F_g > F_up) && (*p_ex_upper < p_upper_threshold)){
            *p_ex_lower = *p_ex_guess;
            *p_ex_guess = *p_ex_upper;
            *p_ex_upper *= 5;
            F_g = F_up;//UAtPFunc(*p_ex_guess,params);
            F_up = UAtPFunc(*p_ex_upper,params);
        }
    }
}



///////////////////////////////////////////////////////////////
/////////// BEGINNING OF BLOCK WITH METHODS RELATED ///////////
/////////// TO THE CALCULATION OF GROWTH RATES      /////////// 
///////////////////////////////////////////////////////////////

/**
 * Calculates and stores runaway growth rates, such as the avalanche growth. 
 * Uses the matched formula from Hesslow et al. NF 59, 084004 (2019) for
 * the critical runaway momentum, which has been generalized to account for 
 * arbitrary inhomogeneous magnetic fields, see DREAM/doc/notes/theory.
 */
void RunawayFluid::CalculateGrowthRates(){
    real_t *n_tot = unknowns->GetUnknownData(id_ntot); 
    for (len_t ir = 0; ir<this->nr; ir++){
        avalancheGrowthRate[ir] = n_tot[ir] * constPreFactor * criticalREMomentumInvSq[ir];
        real_t pc = criticalREMomentum[ir]; 
//        if(pc!=std::numeric_limits<real_t>::infinity()){
 //           real_t gamma_crit = sqrt( 1 + pc*pc );
            tritiumRate[ir] = evaluateTritiumRate(pc);
            comptonRate[ir] = n_tot[ir]*evaluateComptonRate(criticalREMomentum[ir],gsl_ad_w);
   //     }
    }
}


/**
 * Returns the runaway rate due to beta decay of tritium. The net runaway rate
 * dnRE/dt is obtained after multiplication by n_tritium.
 */
real_t RunawayFluid::evaluateTritiumRate(real_t pc){
    if(isinf(pc))
        return 0;
    real_t gamma_c = sqrt(1+pc*pc);
    real_t gammaMinusOne = pc*pc/(gamma_c+1);
    real_t w = Constants::mc2inEV * gammaMinusOne / tritiumDecayEnergyEV;
    real_t fracAbovePc = 1 + sqrt(w)*( -(35/8)*w + (21/4)*w*w - (15/8)*w*w*w);
    if(fracAbovePc < 0)
        return 0;

    return log(2) /tritiumHalfLife * fracAbovePc;
}


/**
 * Returns the total cross section for Compton scattering into p>pc due to incident photons 
 * of energy Eg (units of mc and mc2). Eq (29) in Martin-Solis NF 2017.
 */
real_t RunawayFluid::evaluateComptonTotalCrossSectionAtP(real_t Eg, real_t pc){
    real_t gamma_c = sqrt(1+pc*pc);
    real_t x = Eg;
    real_t Wc = pc*pc/(gamma_c+1); // = gamma_c-1
    real_t cc = 1 - 1/Eg * Wc /( Eg - Wc );
    return M_PI * Constants::r0 * Constants::r0 * ( (x*x-2*x-2)/(x*x*x) * log( (1+2*x)/( 1+x*(1-cc) ) ) 
        + 1/(2*x) * ( 1/( (1+x*(1-cc))*(1+x*(1-cc)) ) - 1/( (1+2*x)*(1+2*x) ) ) 
        - 1/(x*x*x) * ( 1 - x - (1+2*x) / (1+x*(1-cc)) - x*cc )   );
}

/**
 * Returns the photon spectral flux density expected for ITER, Eq (24) in Martin-Solis NF 2017.
 * TODO: provide settings to specify the photon flux density.
 * TODO: actually the flux density should be such that
 *   integral(evaluateComptonPhotonFluxSpectrum,Eg,0,inf) = 1e18
 */
real_t RunawayFluid::evaluateComptonPhotonFluxSpectrum(real_t Eg){
    real_t ITERPhotonFluxDensity = 1e18; // 1/m^2s
    real_t z = (1.2 + log(Eg * Constants::mc2inEV/1e6) ) / 0.8;
    return ITERPhotonFluxDensity * exp( - exp(z) - z + 1 );
}


/**
 * Returns the integrand appearing in the evaluation of the total production rate integral (flux density x cross section ) 
 */
struct ComptonParam {real_t pc;};
real_t ComptonIntegrandFunc(real_t Eg, void *par){
    struct ComptonParam *params = (struct ComptonParam *) par;
    
    real_t pc = params->pc;

    return RunawayFluid::evaluateComptonPhotonFluxSpectrum(Eg) * RunawayFluid::evaluateComptonTotalCrossSectionAtP(Eg,pc);
}

/**
 * Returns the runaway rate due to Compton scattering on gamma rays. The net runaway rate
 * dnRE/dt is obtained after multiplication by the total electron density n_tot.
 */
real_t RunawayFluid::evaluateComptonRate(real_t pc,gsl_integration_workspace *gsl_ad_w){
    if(isinf(pc))
        return 0;
    real_t gamma_c = sqrt(1+pc*pc);
    real_t gammacMinusOne = pc*pc/(gamma_c+1); // = gamma_c-1
    struct ComptonParam  params= {pc};
    gsl_function ComptonFunc;
    ComptonFunc.function = &(ComptonIntegrandFunc);
    ComptonFunc.params = &params;

    real_t Eg_min = (pc + gammacMinusOne) /2;
    real_t valIntegral;
    // qagiu assumes an infinite upper boundary
    real_t epsrel = 1e-4;
    real_t epsabs;
    gsl_integration_qagiu(&ComptonFunc, Eg_min , 0, epsrel, 1000, gsl_ad_w, &valIntegral, &epsabs);
    return valIntegral;
}


/**
 * Parameter struct used for the evaluation of pStarFunction.
 */
struct pStarFuncParams {real_t constTerm; len_t ir; RunawayFluid *rf;CollisionQuantity::collqty_settings *collSettingsForPc;};

/**
 * Returns the value of the function whose root (with respect to momentum p) 
 * corresponds to the critical runaway momentum.
 */
real_t RunawayFluid::pStarFunction(real_t p, void *par){
    struct pStarFuncParams *params = (struct pStarFuncParams *) par;
    CollisionQuantity::collqty_settings *collSettingsForPc = params->collSettingsForPc;
    real_t constTerm = params->constTerm;
    real_t ir = params->ir;
    RunawayFluid *rf = params->rf;
    return sqrt(sqrt(rf->evaluateBarNuSNuDAtP(ir,p,collSettingsForPc)))/constTerm -  p;
}

/**
 * Calculates and stores the critical runaway momentum. We separately store 1/p^2, since this is the factor
 * entering the avalanche growth rate, and our model will allow it to go negative to capture runaway decay.
 */
void RunawayFluid::CalculateCriticalMomentum(){
    real_t E, constTerm;
    real_t effectivePassingFraction;
    gsl_function gsl_func;
    pStarFuncParams pStar_params;
    real_t pLo, pUp, pStar;
    real_t *E_term = unknowns->GetUnknownData(id_Eterm); 
    for(len_t ir=0; ir<this->nr; ir++){
        if(E_term[ir] > effectiveCriticalField[ir])
            E =  Constants::ec * E_term[ir] /(Constants::me * Constants::c);
        else
            E =  Constants::ec * effectiveCriticalField[ir] /(Constants::me * Constants::c);

        /*
        Chooses whether trapping effects are accounted for in growth rates via setting 
        (could imagine another setting where you go smoothly from one to the other as 
        t_orbit/t_coll_at_pstar goes from <<1 to >>1)
        */
        if(collQtySettings->pstar_mode == OptionConstants::COLLQTY_PSTAR_MODE_COLLISIONAL){
            effectivePassingFraction = 1;
        } else if(collQtySettings->pstar_mode == OptionConstants::COLLQTY_PSTAR_MODE_COLLISIONLESS){
            effectivePassingFraction = rGrid->GetEffPassFrac(ir);
        }
        constTerm = sqrt(sqrt(E*E * effectivePassingFraction));

        pStar_params = {constTerm,ir,this, collSettingsForPc}; 
        gsl_func.function = &(pStarFunction);
        gsl_func.params = &pStar_params;

        // Estimate bounds on pStar assuming the limits of complete and no screening. Note that nuSHat and nuDHat are independent of p
        CollisionQuantity::collqty_settings collSetCompScreen;
        collSetCompScreen = *collSettingsForPc;
        collSetCompScreen.collfreq_type = OptionConstants::COLLQTY_COLLISION_FREQUENCY_TYPE_COMPLETELY_SCREENED;
        CollisionQuantity::collqty_settings collSetNoScreen;
        collSetNoScreen = *collSettingsForPc;
        collSetNoScreen.collfreq_type = OptionConstants::COLLQTY_COLLISION_FREQUENCY_TYPE_NON_SCREENED;
        real_t nuSHat_COMPSCREEN = evaluateNuSHat(ir,1,&collSetCompScreen);
        real_t nuDHat_COMPSCREEN = evaluateNuDHat(ir,1,&collSetCompScreen);
        real_t nuSHat_NOSCREEN = evaluateNuSHat(ir,1,&collSetNoScreen);
        real_t nuDHat_NOSCREEN = evaluateNuDHat(ir,1,&collSetNoScreen);
        pc_COMPLETESCREENING[ir] = sqrt(sqrt(nuSHat_COMPSCREEN*nuDHat_COMPSCREEN)/E);
        pc_NOSCREENING[ir] = sqrt( sqrt(nuSHat_NOSCREEN*nuDHat_NOSCREEN) /E );

        pLo = pc_COMPLETESCREENING[ir];
        pUp = pc_NOSCREENING[ir];
        FindInterval(&pLo,&pUp, gsl_func);
        FindRoot(pLo,pUp, &pStar, gsl_func,fsolve);

        // Set critical RE momentum so that 1/pc^2 = (E-Eceff)/sqrt(NuSbar(NuDbar + 4*NuSbar))
        real_t nuSHat = evaluateNuSHat(ir,pStar,collSettingsForPc);
        real_t nuDHat = evaluateNuDHat(ir,pStar,collSettingsForPc);

        real_t EMinusEceff = Constants::ec * (E_term[ir] - effectiveCriticalField[ir]) /(Constants::me * Constants::c);
        real_t nuSnuDTerm = nuSHat*(nuDHat + 4*nuSHat) ;
        criticalREMomentumInvSq[ir] = EMinusEceff*sqrt(effectivePassingFraction) / sqrt(nuSnuDTerm);

        if (EMinusEceff<=0)
            criticalREMomentum[ir] = std::numeric_limits<real_t>::infinity() ; // should make growth rates zero
        else
            criticalREMomentum[ir] = 1/sqrt(criticalREMomentumInvSq[ir]);
    }
}
    
/**
 *  Returns nuS*p^3/gamma^2, which is constant for ideal plasmas. (only lnL energy dependence)
 */
real_t RunawayFluid::evaluateNuSHat(len_t ir, real_t p, CollisionQuantity::collqty_settings *inSettings){
    OptionConstants::collqty_collfreq_mode collfreq_mode = collQtySettings->collfreq_mode;
    return constPreFactor * nuS->evaluateAtP(ir,p,inSettings) / nuS->evaluatePreFactorAtP(p,collfreq_mode);
}
/** 
 * Returns nuD*p^3/gamma, which is constant for ideal plasmas. (only lnL energy dependence)
 */
real_t RunawayFluid::evaluateNuDHat(len_t ir, real_t p, CollisionQuantity::collqty_settings *inSettings){
    OptionConstants::collqty_collfreq_mode collfreq_mode = collQtySettings->collfreq_mode;
    return constPreFactor * nuD->evaluateAtP(ir,p,inSettings) / nuD->evaluatePreFactorAtP(p,collfreq_mode);
}

/**
 * Returns nuS*nuD*p^6/gamma^3, which is constant for ideal plasmas. (only lnL energy dependence)
 */
real_t RunawayFluid::evaluateBarNuSNuDAtP(len_t ir, real_t p, CollisionQuantity::collqty_settings *inSettings){
    real_t nuSHat = evaluateNuSHat(ir,p,inSettings);
    real_t nuDHat = evaluateNuDHat(ir,p,inSettings);
    return nuSHat * nuDHat;
}

/**
 * Allocate all stored quantities. 
 */
void RunawayFluid::AllocateQuantities(){
    DeallocateQuantities();
    Ec_free  = new real_t[nr];
    Ec_tot   = new real_t[nr];
    tauEERel = new real_t[nr];
    tauEETh  = new real_t[nr];
    EDreic   = new real_t[nr];
    effectiveCriticalField  = new real_t[nr];
    criticalREMomentum      = new real_t[nr];
    criticalREMomentumInvSq = new real_t[nr];
    pc_COMPLETESCREENING    = new real_t[nr];
    pc_NOSCREENING          = new real_t[nr];
    avalancheGrowthRate     = new real_t[nr];
    tritiumRate = new real_t[nr];
    comptonRate = new real_t[nr];



}

/**
 * Deallocate all quantities
 */
void RunawayFluid::DeallocateQuantities(){
    if(Ec_free != nullptr){
        delete [] Ec_free;
        delete [] Ec_tot;
        delete [] tauEERel;
        delete [] tauEETh;
        delete [] EDreic;
        delete [] effectiveCriticalField;
        delete [] criticalREMomentum;
        delete [] pc_COMPLETESCREENING;
        delete [] pc_NOSCREENING;
        delete [] avalancheGrowthRate;
        delete [] tritiumRate;
        delete [] comptonRate;

    }
}





/**
 * Returns the Braams-Karney electric conductivity of a relativistic plasma.
 */
real_t RunawayFluid::evaluateSauterElectricConductivity(len_t ir, real_t Zeff){
    return evaluateBraamsElectricConductivity(ir,Zeff) * evaluateNeoclassicalConductivityCorrection(ir,Zeff);
}



/**
 * Returns the Braams-Karney electric conductivity of a relativistic plasma.
 */
real_t RunawayFluid::evaluateBraamsElectricConductivity(len_t ir, real_t Zeff){
    len_t id_Tcold = unknowns->GetUnknownID(OptionConstants::UQTY_T_COLD);
    real_t *T_cold = unknowns->GetUnknownData(id_Tcold);
    const real_t T_SI = T_cold[ir] * Constants::ec;
//    const real_t *Zeff = ionHandler->evaluateZeff();

    real_t sigmaBar = gsl_interp2d_eval(gsl_cond, conductivityTmc2, conductivityX, conductivityBraams, 
                T_SI / (Constants::me * Constants::c * Constants::c), 1/(1+Zeff), gsl_xacc, gsl_yacc  );
    
    real_t BraamsConductivity = 4*M_PI*Constants::eps0*Constants::eps0 * T_SI*sqrt(T_SI) / 
            (sqrt(Constants::me) * Constants::ec * Constants::ec * lnLambdaEE->GetLnLambdaT(ir) ) * sigmaBar;
    return BraamsConductivity;
}


/**
 * Returns the correction to the Spitzer conductivity, valid in all collisionality regimes,
 * taken from O Sauter, C Angioni and Y R Lin-Liu, Phys Plasmas 6, 2834 (1999).
 */
real_t RunawayFluid::evaluateNeoclassicalConductivityCorrection(len_t ir, real_t Zeff, bool collisionLess){
    real_t ft = 1 - rGrid->GetEffPassFrac(ir);
    
    
    real_t X = ft;
    const real_t R0 = rGrid->GetR0();
    if(isinf(R0))
        X = 0;
    else if(!collisionLess){
        // qR0 is the safety factor multiplied by R0
        const real_t qR0 =  rGrid->GetVpVol(ir)*rGrid->GetVpVol(ir)*rGrid->GetBTorG(ir)*rGrid->GetFSA_1OverR2(ir)*rGrid->GetFSA_NablaR2OverR2(ir)
             / (4*M_PI*M_PI*Constants::mu0*unknowns->GetUnknownData(unknowns->GetUnknownID(OptionConstants::UQTY_I_P))[ir]);     
        real_t *T_cold = unknowns->GetUnknownData(id_Tcold);
        real_t *n_cold = unknowns->GetUnknownData(id_ncold);
        real_t TkeV = T_cold[ir]/1000;
        real_t eps = rGrid->GetR(ir)/R0;
        real_t nuEStar = 0.012*n_cold[ir]*Zeff * qR0/(eps*sqrt(eps) * TkeV*TkeV);

        X /= 1 + (0.55-0.1*ft)*sqrt(nuEStar) + 0.45*(1-ft)*nuEStar/(Zeff*sqrt(Zeff)) ;
    }
    return 1 - (1+0.36/Zeff)*X + X*X/Zeff * (0.59-0.23*X);
}



/**
 * Public method used mainly for benchmarking: evaluates the pitch-averaged friction function -U 
 */
real_t RunawayFluid::testEvalU(len_t ir, real_t p, real_t Eterm, bool useApproximateMethod, CollisionQuantity::collqty_settings *inSettings){
    std::function<real_t(real_t,real_t,real_t)> Func = [](real_t,real_t,real_t){return 0;};
    real_t p_ex_lo = 0, p_ex_up = 0;
    gsl_integration_workspace *gsl_ad_w = gsl_integration_workspace_alloc(1000);
    const gsl_min_fminimizer_type *fmin_type = gsl_min_fminimizer_brent;
    gsl_min_fminimizer *fmin = gsl_min_fminimizer_alloc(fmin_type);

    struct UContributionParams params = {rGrid, this, nuS,nuD, ir, p, FVM::FLUXGRIDTYPE_DISTRIBUTION, Eterm, Func, gsl_ad_w,
                    fmin, p_ex_lo, p_ex_up,useApproximateMethod,inSettings};
    return UAtPFunc(p,&params);
}
