/**
 * Implementation of a class which handles the calculation of the 
 * pitch-angle scattering frequency nu_D. It is defined such that 
 * the D^(xi,xi) component of the collision operator is given by 
 * (1-xi^2)nu_D/2.
*/

/**
 * The calculations of the electron-ion contribution are based on Eq (2.22) from
 * L Hesslow et al., Generalized collision operator for fast electrons
 * interacting with partially ionized impurities, J Plasma Phys 84 (2018).
 * The relativistic thermal ee contribution is based on the expressions given in
 * Pike & Rose, Dynamical friction in a relativistic plasma, Phys Rev E 89 (2014).
 * The non-linear contribution corresponds to the isotropic component of the
 * non-relativistic operator following Rosenbluth, Macdonald & Judd, Phys Rev (1957),
 * and is described in doc/notes/theory.pdf Appendix B.
 */
#include "DREAM/Equations/PitchScatterFrequency.hpp"
#include "DREAM/NotImplementedException.hpp"
#include "FVM/FVMException.hpp"

using namespace DREAM;

/**
 * Effective ion charge parameters from Table 1 of the Hesslow (2018) paper.
 */
const len_t PitchScatterFrequency::ionSizeAj_len = 55; 
const real_t PitchScatterFrequency::ionSizeAj_data[ionSizeAj_len] = { 0.631757734322417, 0.449864664424796, 0.580073385681175, 0.417413282378673, 0.244965367639212, 0.213757911761448, 0.523908484242040, 0.432318176055981, 0.347483799585738, 0.256926098516580, 0.153148466772533, 0.140508604177553, 0.492749302776189, 0.419791849305259, 0.353418389488286, 0.288707775999513, 0.215438905215275, 0.129010899184783, 0.119987816515379, 0.403855887938967, 0.366602498048607, 0.329462647492495, 0.293062618368335, 0.259424839110224, 0.226161504309134, 0.190841656429844, 0.144834685411878, 0.087561370494245, 0.083302176729104, 0.351554934261205, 0.328774241757188, 0.305994557639981, 0.283122417984972, 0.260975850956140, 0.238925715853581, 0.216494264086975, 0.194295316086760, 0.171699132959493, 0.161221485564969, 0.150642403738712, 0.139526182041846, 0.128059339783537, 0.115255069413773, 0.099875435538094, 0.077085983503479, 0.047108093547224, 0.045962185039177, 0.235824746357894, 0.230045911002090, 0.224217341261303, 0.215062179624586, 0.118920957451653, 0.091511805821898, 0.067255603181663, 0.045824624741631 };
const real_t PitchScatterFrequency::ionSizeAj_Zs[ionSizeAj_len] = { 2, 2, 4, 4, 4, 4, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 54, 54, 54, 74, 74, 74, 74, 74 };
const real_t PitchScatterFrequency::ionSizeAj_Z0s[ionSizeAj_len] = { 0, 1, 0, 1, 2, 3, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 1, 2, 3, 0, 30, 40, 50, 60 };

/**
 * Constructor
 */
PitchScatterFrequency::PitchScatterFrequency(FVM::Grid *g, FVM::UnknownQuantityHandler *u, IonHandler *ih,  
                CoulombLogarithm *lnLei, CoulombLogarithm *lnLee,
                enum OptionConstants::momentumgrid_type mgtype,  struct collqty_settings *cqset)
                : CollisionFrequency(g,u,ih,lnLee,lnLei,mgtype,cqset){
    hasIonTerm = true;
}

PitchScatterFrequency::~PitchScatterFrequency(){
    DeallocateCollisionQuantities();
    DeallocatePartialQuantities();
}

/**
 * Evaluates the "Kirillov-model" Thomas-Fermi formula, Equation (2.25) in the Hesslow paper. 
 */
real_t PitchScatterFrequency::evaluateScreenedTermAtP(len_t iz, len_t Z0, real_t p){
    len_t ind = ionIndex[iz][Z0];
    len_t Z = Zs[iz];
    real_t a = atomicParameter[ind];
    real_t x = p*a*sqrt(p*a); 
    return 2.0/3.0 * ((Z*Z-Z0*Z0)*log(1+x) - (Z-Z0)*(Z-Z0)*x/(1+x) );
}

/**
 * Evaluates the ion partial contribution to the collision frequency.
 */
real_t PitchScatterFrequency::evaluateIonTermAtP(len_t /*iz*/, len_t /*Z0*/, real_t /*p*/){
    return 1;
}

real_t PitchScatterFrequency::evaluatePreFactorAtP(real_t p, OptionConstants::collqty_collfreq_mode collfreq_mode){
    if(p==0) 
        return 0; 
    else if (collfreq_mode != OptionConstants::COLLQTY_COLLISION_FREQUENCY_MODE_ULTRA_RELATIVISTIC)
        return constPreFactor * sqrt(1+p*p)/(p*p*p);
    else 
        return constPreFactor / (p*p);
}



/**
 * Helper function to calculate the electron partial contribution to the frequency
 */
real_t PitchScatterFrequency::evaluateElectronTermAtP(len_t ir, real_t p,OptionConstants::collqty_collfreq_mode collfreq_mode){
    if (collfreq_mode==OptionConstants::COLLQTY_COLLISION_FREQUENCY_MODE_FULL){
        if(p==0)
            return 0;
        real_t p2 = p*p;
        real_t *T_cold = unknowns->GetUnknownData(id_Tcold);
        real_t Theta,M;
        real_t gamma = sqrt(1+p2);
        Theta = T_cold[ir] / Constants::mc2inEV;
        M = 0;
        M += (p2*gamma*gamma + Theta*Theta)*evaluatePsi0(ir,p);
        M += Theta*(2*p2*p2 - 1)*evaluatePsi1(ir,p);
        M += gamma*Theta * ( 1 + Theta*(2*p2-1)*p*exp( -(gamma-1)/Theta ) );
        M /= gamma*gamma*p*p*evaluateExp1OverThetaK(Theta,2.0);
        return  M;
    } else
        return 1;
}


/**
 *  Calculates a Rosenbluth potential matrix defined such that when it is muliplied
 * by the f_hot distribution vector, yields the pitch-angle scattering frequency.
 */
void PitchScatterFrequency::calculateIsotropicNonlinearOperatorMatrix(){
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
        nonlinearMat[i][0] = (4*M_PI/3) * constPreFactor / p_f[i]*( (p[1]-p[0])/2*(3-p2/p2f) + p[0]*(1-p2/(5*p2f) ))*p2/p2f;
        for (len_t ip = 1; ip < i-1; ip++){
            p2 = p[ip]*p[ip];
            nonlinearMat[i][ip] = (4*M_PI/3) * constPreFactor / p_f[i] * trapzWeights[ip]*p2/p2f *(3-p2/p2f) ;
        } 
        p2 = p[i-1]*p[i-1];
        weightsIm1 = (p[i-1]-p[i-2])/2 + (p_f[i]-p[i-1])/(p[i]-p[i-1])*( (2*p[i]-p_f[i]-p[i-1])/2 );
        nonlinearMat[i][i-1] = (4*M_PI/3) * constPreFactor / p_f[i] * weightsIm1*p2/p2f *(3-p2/p2f) ;
        p2 = p[i]*p[i];
        weightsI = (p_f[i]-p[i-1])*(p_f[i]-p[i-1])/(p[i]-p[i-1]);
        nonlinearMat[i][i] = (4*M_PI/3) * constPreFactor / p_f[i] * weightsI*p2/p2f *(3-p2/p2f) ;

        // add contribution from p'>p terms near p'=p
        p2 = p[i-1]*p[i-1];
        weightsIm1 = (1.0/2)*(p[i]-p_f[i])*(p[i]-p_f[i])/(p[i]-p[i-1]);
        nonlinearMat[i][i-1] += (8*M_PI/3) * constPreFactor / p_f[i] * weightsIm1*p[i-1]/p2f;
        p2 = p[i]*p[i];
        weightsI = (p[i+1]-p[i])/2 + (1.0/2)*(p[i]-p_f[i])*(p_f[i]+p[i]-2*p[i-1])/(p[i]-p[i-1]);
        nonlinearMat[i][i] += (8*M_PI/3) * constPreFactor * weightsI * p[i]/p2f;


        for (len_t ip = i+1; ip < np1-1; ip++){
            nonlinearMat[i][ip] = (8*M_PI/3) * constPreFactor * trapzWeights[ip]*p[ip]/p2f ;
        } 
        real_t weightsEnd = (p[np1-1]-p[np1-2])/2;
        nonlinearMat[i][np1-1] = (8*M_PI/3) * constPreFactor * weightsEnd*p[np1-1]/p2f ;
        

    }
}

/**
 * Returns the effective ion size parameter; when available, takes data
 * from DFT calculations as given in Table 1 in the Hesslow (2018) paper,
 * otherwise uses the analytical approximation presented in Equation (2.28)
 * in the same paper.  
 */
real_t PitchScatterFrequency::GetAtomicParameter(len_t iz, len_t Z0){
    len_t Z = ionHandler->GetZ(iz);
    // Fetch DFT-calculated value from table if it exists:
    for (len_t n=0; n<ionSizeAj_len; n++)
        if( Z==ionSizeAj_Zs[n] && (Z0==ionSizeAj_Z0s[n]) )
            return 2/Constants::alpha*ionSizeAj_data[n];

    // If DFT-data is missing, use Kirillov's model:
    return 2/Constants::alpha * pow(9*M_PI,1./3) / 4 * pow(Z-Z0,2./3) / Z;

}

