/**
 * Set up a collision quantity handler.
 */

#include "DREAM/Settings/SimulationGenerator.hpp"


using namespace DREAM;
using namespace std;


#define MODNAME "collisions"

/**
 * Construct a CollisionQuantityHandler object for the specified
 * grid object. Read settings from the named section of the settings.
 *
 * name:     Name of settings section to load options from.
 * grid:     Grid object for which to construct the collision handler.
 * unknowns: List of unknowns in the associated equation system.
 * s:        Settings describing how to construct the collision handler.
 */
RunawayFluid *SimulationGenerator::ConstructRunawayFluid(FVM::Grid *g,
    FVM::UnknownQuantityHandler *unknowns, IonHandler *ih, 
    OptionConstants::momentumgrid_type gridtype, Settings *s
) {
    struct CollisionQuantity::collqty_settings *cq =
        new CollisionQuantity::collqty_settings;

    cq->collfreq_type = (enum OptionConstants::collqty_collfreq_type)s->GetInteger(MODNAME "/collfreq_type");
    cq->collfreq_mode = (enum OptionConstants::collqty_collfreq_mode)s->GetInteger(MODNAME "/collfreq_mode");
    cq->lnL_type      = (enum OptionConstants::collqty_lnLambda_type)s->GetInteger(MODNAME "/lnlambda");
    cq->bremsstrahlung_mode = (enum OptionConstants::eqterm_bremsstrahlung_mode)s->GetInteger(MODNAME "/bremsstrahlung_mode");
    cq->pstar_mode = (enum OptionConstants::collqty_pstar_mode)s->GetInteger(MODNAME "/pstar_mode");
    
    CoulombLogarithm *lnLEE = new CoulombLogarithm(g,unknowns,ih,gridtype,cq,CollisionQuantity::LNLAMBDATYPE_EE);
    CoulombLogarithm *lnLEI = new CoulombLogarithm(g,unknowns,ih,gridtype,cq,CollisionQuantity::LNLAMBDATYPE_EI);
    SlowingDownFrequency *nuS = new SlowingDownFrequency(g,unknowns,ih,lnLEE,lnLEI,gridtype,cq);
    PitchScatterFrequency *nuD = new PitchScatterFrequency(g,unknowns,ih,lnLEI,lnLEE,gridtype,cq);

    RunawayFluid *REF = new RunawayFluid(g, unknowns, nuS, nuD, lnLEE, lnLEI, cq, ih);
    
    return REF;
}

