/**
 * Implementation of the electric field diffusion term in the kinetic equation,
 * which is used for hot-tail like grids.
 */

#include "DREAM/Equations/Kinetic/ElectricFieldDiffusionTerm.hpp"


using namespace DREAM;

/**
 * Constructor.
 */
ElectricFieldDiffusionTerm::ElectricFieldDiffusionTerm(FVM::Grid *g, CollisionQuantityHandler *cqh, FVM::UnknownQuantityHandler *unknowns)
    : FVM::DiffusionTerm(g) {
        this->nuD = cqh->GetNuD();
        this->id_Eterm  = unknowns->GetUnknownID(OptionConstants::UQTY_E_FIELD); // E term should be <E*B>/sqrt(<B^2>)
    
        AddUnknownForJacobian(unknowns, unknowns->GetUnknownID(OptionConstants::UQTY_E_FIELD));
        AddUnknownForJacobian(unknowns, unknowns->GetUnknownID(OptionConstants::UQTY_N_COLD));
        AddUnknownForJacobian(unknowns, unknowns->GetUnknownID(OptionConstants::UQTY_ION_SPECIES));

}


/**
 * Build the coefficients of this diffusion term. Realistically only used when np2 = 1, but let's keep it general.
 */
void ElectricFieldDiffusionTerm::Rebuild(
    const real_t, const real_t, FVM::UnknownQuantityHandler *x
){
    const len_t nr = grid->GetNr();
    E_term = x->GetUnknownData(id_Eterm);
    real_t *const *nu_D_f1 = nuD->GetValue_f1();
    real_t E;
    for (len_t ir = 0; ir < nr; ir++) {
        auto *mg = grid->GetMomentumGrid(ir);
        const len_t np1 = mg->GetNp1();
        const len_t np2 = mg->GetNp2();        
        E = Constants::ec * E_term[ir] /(Constants::me * Constants::c);
        for (len_t j = 0; j < np2; j++) {
            // sum over i from 1, assume nu_D(p_f0) = inf
            for (len_t i = 1; i < np1+1; i++) { 
                D11(ir, i, j) +=  1.0/3.0
                    * grid->GetRadialGrid()->GetEffPassFrac(ir) 
                    * E * E / nu_D_f1[ir][j*(np1+1)+i];  
            }
        }
    }
}



// Set jacobian of the diffusion coefficients for this diffusion term
void ElectricFieldDiffusionTerm::SetPartialDiffusionTerm(len_t derivId, len_t nMultiples){
    ResetDifferentiationCoefficients();    
    real_t *const *nu_D_f1 = nuD->GetValue_f1();
    real_t E;

    // Derivative with respect to E_term
    if(derivId == id_Eterm){
        for (len_t ir = 0; ir < nr; ir++) {
            const len_t np1 = n1[ir];
            const len_t np2 = n2[ir];        
            E = Constants::ec * E_term[ir] /(Constants::me * Constants::c);
            len_t dE = Constants::ec /(Constants::me * Constants::c);
            for (len_t j = 0; j < np2; j++) {
                for (len_t i = 1; i < np1+1; i++) {
                    dD11(ir, i, j, 0) =  1.0/3.0
                        * grid->GetRadialGrid()->GetEffPassFrac(ir) 
                        * 2.0*E*dE / nu_D_f1[ir][j*(np1+1)+i];  
                }
            }
        }
    // Derivative with respect to n_cold or n_i
    } else {
        const real_t *dNuD_f1 = nuD->GetUnknownPartialContribution(derivId, FVM::FLUXGRIDTYPE_P1);
        len_t offset = 0;
        for(len_t n=0; n<nMultiples; n++){
            for (len_t ir = 0; ir < nr; ir++) {
                const len_t np1 = n1[ir];
                const len_t np2 = n2[ir];        
                E = Constants::ec * E_term[ir] /(Constants::me * Constants::c);
                for (len_t j = 0; j < np2; j++) {
                    for (len_t i = 1; i < np1+1; i++) {
                        dD11(ir, i, j, n) =  -1.0/3.0
                            * grid->GetRadialGrid()->GetEffPassFrac(ir) 
                            * E * E / (nu_D_f1[ir][j*(np1+1)+i]*nu_D_f1[ir][j*(np1+1)+i])
                            * dNuD_f1[offset + (np1+1)*j + i];  
                    }
                }
                offset += (np1+1)*np2;
            }
        }
    }
}

