#include "DREAM/Equations/Fluid/RadiatedPowerTerm.hpp"


/**
 * Implementation of a class which represents the 
 * radiated power as calculated with rate coefficients
 * from the ADAS database (PLT corresponds to line
 * and PRB to brems and recombination radiated power).
 * The term is of the form n_e * sum_i n_i L_i, summed over all
 * ion species i. In the semi-implicit solver, n_e is the "unknown"
 * evaluated at the next time step and n_i L_i coefficients.
 * We ignore the Jacobian with respect to L_i(n,T) and capture only the
 * n_e and n_i contributions.
 */


using namespace DREAM;


RadiatedPowerTerm::RadiatedPowerTerm(FVM::Grid* g, FVM::UnknownQuantityHandler *u, IonHandler *ionHandler, ADAS *adas) 
    : FVM::DiagonalComplexTerm(g,u) 
{
    this->adas = adas;
    this->ionHandler = ionHandler;

    this->id_ncold = unknowns->GetUnknownID(OptionConstants::UQTY_N_COLD);
    this->id_Tcold = unknowns->GetUnknownID(OptionConstants::UQTY_T_COLD);
    this->id_ni    = unknowns->GetUnknownID(OptionConstants::UQTY_ION_SPECIES);

    AddUnknownForJacobian(unknowns, id_ncold);
    AddUnknownForJacobian(unknowns, id_ni);
    AddUnknownForJacobian(unknowns, id_Tcold);
    
}


void RadiatedPowerTerm::SetWeights() 
{
    len_t NCells = grid->GetNCells();
    len_t nZ = ionHandler->GetNZ();
    const len_t *Zs = ionHandler->GetZs();
    
    real_t *n_cold = unknowns->GetUnknownData(id_ncold);
    real_t *T_cold = unknowns->GetUnknownData(id_Tcold);
    real_t *n_i    = unknowns->GetUnknownData(id_ni);
    

    for (len_t i = 0; i < NCells; i++)
            weights[i] = 0;


    for(len_t iz = 0; iz<nZ; iz++){
        ADASRateInterpolator *PLT_interper = adas->GetPLT(Zs[iz]);
        ADASRateInterpolator *PRB_interper = adas->GetPRB(Zs[iz]);
        for(len_t Z0 = 0; Z0<=Zs[iz]; Z0++){
            len_t nMultiple = ionHandler->GetIndex(iz,Z0);
            for (len_t i = 0; i < NCells; i++){
                real_t Li =  PLT_interper->Eval(Z0, n_cold[i], T_cold[i])
                            + PRB_interper->Eval(Z0, n_cold[i], T_cold[i]);
                real_t ni = n_i[nMultiple*NCells + i];
                weights[i] += ni*Li;


/*
                len_t ind = NCells-1;
                if( (Zs[iz]==1) && (Z0==1) && (i==ind))
                    std::cout << "Recombination radiated power: " << -ni*Li*n_cold[ind] << std::endl;                    

                if( (Zs[iz]==18) && (Z0==0)  && (i==ind)){                    
                    len_t id_E = unknowns->GetUnknownID(OptionConstants::UQTY_E_FIELD);
                    len_t id_j_ohm = unknowns->GetUnknownID(OptionConstants::UQTY_J_OHM);
                    len_t id_Wc = unknowns->GetUnknownID(OptionConstants::UQTY_W_COLD);
                    real_t E = unknowns->GetUnknownData(id_E)[ind];
                    real_t j_ohm = unknowns->GetUnknownData(id_j_ohm)[ind];
                    real_t Wc = unknowns->GetUnknownData(id_Wc)[ind];
                    std::cout << "Line radiated power: " << -ni*Li*n_cold[ind] << std::endl;
                    std::cout << "Ohmic heating term: " << E*j_ohm << std::endl; 
                    std::cout << "E: " << E << std::endl;
                    std::cout << "j_ohm: " << j_ohm << std::endl;
                    std::cout << "T: " << T_cold[ind] << std::endl;
                    std::cout << "Wc: " << Wc << std::endl;
                }
  */

            }
        }
    }
}

void RadiatedPowerTerm::SetDiffWeights(len_t derivId, len_t /*nMultiples*/){
    len_t NCells = grid->GetNCells();
    len_t nZ = ionHandler->GetNZ();
    const len_t *Zs = ionHandler->GetZs();

    real_t *n_cold = unknowns->GetUnknownData(id_ncold);
    real_t *T_cold = unknowns->GetUnknownData(id_Tcold);
    real_t *n_i    = unknowns->GetUnknownData(id_ni);

    if(derivId == id_ni){

        for(len_t iz = 0; iz<nZ; iz++){
            ADASRateInterpolator *PLT_interper = adas->GetPLT(Zs[iz]);
            ADASRateInterpolator *PRB_interper = adas->GetPRB(Zs[iz]);
            for(len_t Z0 = 0; Z0<=Zs[iz]; Z0++){
                len_t nMultiple = ionHandler->GetIndex(iz,Z0);
                for (len_t i = 0; i < NCells; i++){
                    real_t Li =  PLT_interper->Eval(Z0, n_cold[i], T_cold[i])
                                + PRB_interper->Eval(Z0, n_cold[i], T_cold[i]);
                    diffWeights[NCells*nMultiple + i] = Li;
                }
            }
        }
    } else if(derivId == id_ncold){
        for(len_t iz = 0; iz<nZ; iz++){
            ADASRateInterpolator *PLT_interper = adas->GetPLT(Zs[iz]);
            ADASRateInterpolator *PRB_interper = adas->GetPRB(Zs[iz]);
            for(len_t Z0 = 0; Z0<=Zs[iz]; Z0++){
                len_t nMultiple = ionHandler->GetIndex(iz,Z0);
                for (len_t i = 0; i < NCells; i++){
                    real_t dLi =  PLT_interper->Eval_deriv_n(Z0, n_cold[i], T_cold[i])
                                + PRB_interper->Eval_deriv_n(Z0, n_cold[i], T_cold[i]);
                    real_t ni = n_i[nMultiple*NCells + i];
                    diffWeights[i] += ni*dLi;
                }
            }
        }

    } else if (derivId == id_Tcold){
        for(len_t iz = 0; iz<nZ; iz++){
            ADASRateInterpolator *PLT_interper = adas->GetPLT(Zs[iz]);
            ADASRateInterpolator *PRB_interper = adas->GetPRB(Zs[iz]);
            for(len_t Z0 = 0; Z0<=Zs[iz]; Z0++){
                len_t nMultiple = ionHandler->GetIndex(iz,Z0);
                for (len_t i = 0; i < NCells; i++){
                    real_t dLi =  PLT_interper->Eval_deriv_T(Z0, n_cold[i], T_cold[i])
                                + PRB_interper->Eval_deriv_T(Z0, n_cold[i], T_cold[i]);
                    real_t ni = n_i[nMultiple*NCells + i];
                    diffWeights[i] += ni*dLi;
                }
            }
        }

    }

}

