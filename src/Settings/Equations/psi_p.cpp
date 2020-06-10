/**
 * Definition of equations relating to the poloidal flux.
 */

#include <iostream>
#include <string>
#include <gsl/gsl_sf_bessel.h>
#include "DREAM/EquationSystem.hpp"
#include "DREAM/Settings/SimulationGenerator.hpp"
#include "FVM/Equation/Equation.hpp"
#include "FVM/Equation/TransientTerm.hpp"
#include "FVM/Interpolator3D.hpp"
#include "FVM/Equation/PrescribedParameter.hpp"
#include "FVM/Equation/WeightedIdentityTerm.hpp"
#include "FVM/Equation/WeightedTransientTerm.hpp"
#include "DREAM/Equations/PoloidalFlux/AmperesLawDiffusionTerm.hpp"
#include "DREAM/Equations/PoloidalFlux/HyperresistiveDiffusionTerm.hpp"

using namespace DREAM;
using namespace std;


/**
 * Implementation of a class which represents the j_||/(B/Bmin) term in Ampere's law.
 */
namespace DREAM {
    class AmperesLawJTotTerm : public FVM::WeightedIdentityTerm {
    protected:
        virtual bool TermDependsOnUnknowns() override {return false;}
    public:
        AmperesLawJTotTerm(FVM::Grid* g) : FVM::WeightedIdentityTerm(g){ this->GridRebuilt();}

        virtual void SetWeights() override {
            len_t offset = 0;
            for (len_t ir = 0; ir < nr; ir++){
                real_t w = - Constants::mu0 * grid->GetRadialGrid()->GetFSA_1OverR2(ir) * grid->GetRadialGrid()->GetBTorG(ir) / grid->GetRadialGrid()->GetBmin(ir);
                for(len_t i = 0; i < n1[ir]; i++)
                    for(len_t j = 0; j < n2[ir]; j++)
                        weights[offset + n1[ir]*j + i] = w;
                offset += n1[ir]*n2[ir];
            }
        }
    };
}


#define MODULENAME "eqsys/psi_p"

/**
 * Construct the equation for the poloidal flux j_|| ~ Laplace(psi)
 * 
 * eqsys: Equation system to put the equation in.
 * s:     Settings object describing how to construct the equation.
 */
void SimulationGenerator::ConstructEquation_psi_p(
    EquationSystem *eqsys, Settings* /* s */
) {
    FVM::Grid *fluidGrid = eqsys->GetFluidGrid();
    FVM::Equation *eqn_j1 = new FVM::Equation(fluidGrid);
    FVM::Equation *eqn_j2 = new FVM::Equation(fluidGrid);


    // weightFunc3 represents -mu0*R0*<B*nabla phi>/Bmin (ie mu0*weightFunc1)
    std::function<real_t(len_t,len_t,len_t)> weightFunc3 = [fluidGrid](len_t ir,len_t, len_t)
        {return - Constants::mu0 * fluidGrid->GetRadialGrid()->GetFSA_1OverR2(ir) * fluidGrid->GetRadialGrid()->GetBTorG(ir) / fluidGrid->GetRadialGrid()->GetBmin(ir);};

    eqn_j1->AddTerm(new AmperesLawJTotTerm(fluidGrid));
    eqn_j2->AddTerm(new AmperesLawDiffusionTerm(fluidGrid));

    /**
     * TODO: Add additional boundary conditions.
     */

    eqsys->SetEquation(OptionConstants::UQTY_POL_FLUX, OptionConstants::UQTY_J_TOT, eqn_j1, "Poloidal flux Ampere's law");
    eqsys->SetEquation(OptionConstants::UQTY_POL_FLUX, OptionConstants::UQTY_POL_FLUX, eqn_j2);

}
