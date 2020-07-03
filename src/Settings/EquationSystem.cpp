
#include <vector>
#include <string>
#include "DREAM/EquationSystem.hpp"
#include "DREAM/PostProcessor.hpp"
#include "DREAM/Settings/Settings.hpp"
#include "DREAM/Settings/SimulationGenerator.hpp"


using namespace DREAM;
using namespace std;


#define EQUATIONSYSTEM "eqsys"
#define INITIALIZATION "init"

/**
 * Define the options which can be set for things
 * related to the equation system.
 *
 * s: Settings object to define the options for.
 */
void SimulationGenerator::DefineOptions_EquationSystem(Settings *s) {
    s->DefineSetting(EQUATIONSYSTEM "/n_cold/type", "Type of equation to use for determining the cold electron density", (int_t)OptionConstants::UQTY_N_COLD_EQN_PRESCRIBED);
    DefineDataRT(EQUATIONSYSTEM "/n_cold", s);
    
//    s->DefineSetting(EQUATIONSYSTEM "/T_cold/type", "Type of equation to use for determining the electron temperature evolution", (int_t)OptionConstants::UQTY_T_COLD_EQN_PRESCRIBED);
//    DefineDataRT(EQUATIONSYSTEM "/T_cold", s);
}

/**
 * Define options for initialization.
 */
void SimulationGenerator::DefineOptions_Initializer(Settings *s) {
    s->DefineSetting(INITIALIZATION "/eqsysignore", "List of unknown quantities to NOT initialize from output file.", (const string)"");
    s->DefineSetting(INITIALIZATION "/filetimeindex", "Time index to take initialization data for from output file.", (int_t)-1);
    s->DefineSetting(INITIALIZATION "/fromfile", "Name of DREAM output file from which simulation should be initialized.", (const string)"");
    s->DefineSetting(INITIALIZATION "/t0", "Simulation at which to initialize the simulation.", (real_t)0.0);
}

/**
 * Construct an equation system, based on the specification
 * in the given 'Settings' object.
 *
 * s:           Settings object specifying how to construct
 *              the equation system.
 * fluidGrid:   Radial grid for the computation.
 * ht_type:     Exact type of the hot-tail momentum grid.
 * hottailGrid: Grid on which the hot-tail electron population
 *              is computed.
 * re_type:     Exact type of the runaway momentum grid.
 * runawayGrid: Grid on which the runaway electron population
 *              is computed.
 *
 * NOTE: The 'hottailGrid' and 'runawayGrid' will be 'nullptr'
 *       if disabled.
 */
EquationSystem *SimulationGenerator::ConstructEquationSystem(
    Settings *s, FVM::Grid *scalarGrid, FVM::Grid *fluidGrid,
    enum OptionConstants::momentumgrid_type ht_type, FVM::Grid *hottailGrid,
    enum OptionConstants::momentumgrid_type re_type, FVM::Grid *runawayGrid,
    ADAS *adas, NIST *nist
) {
    EquationSystem *eqsys = new EquationSystem(scalarGrid, fluidGrid, ht_type, hottailGrid, re_type, runawayGrid);

    // Initialize from previous simulation output?
    const real_t t0 = ConstructInitializer(eqsys, s);

    // Construct the time stepper
    ConstructTimeStepper(eqsys, s);

    // Construct unknowns
    ConstructUnknowns(eqsys, s, scalarGrid, fluidGrid, hottailGrid, runawayGrid);

    // Construct equations according to settings
    ConstructEquations(eqsys, s, adas, nist);

    // Construct the "other" quantity handler
    ConstructOtherQuantityHandler(eqsys, s);

    // Figure out which unknowns must be part of the matrix,
    // and set initial values for those quantities which don't
    // yet have an initial value.
    eqsys->ProcessSystem(t0);

    // Construct solver (must be done after processing equation system,
    // since we need to know which unknowns are "non-trivial",
    // i.e. need to show up in the solver matrices)
    ConstructSolver(eqsys, s);

    return eqsys;
}

/**
 * Set the equations of the equation system.
 *
 * eqsys: Equation system to define quantities in.
 * s:     Settings object specifying how to construct
 *        the equation system.
 * adas:  ADAS database object.
 * nist:  NIST database object.
 *
 * NOTE: The 'hottailGrid' and 'runawayGrid' will be 'nullptr'
 *       if disabled.
 */
void SimulationGenerator::ConstructEquations(
    EquationSystem *eqsys, Settings *s, ADAS *adas, NIST *nist
) {
    FVM::Grid *hottailGrid = eqsys->GetHotTailGrid();
    FVM::Grid *runawayGrid = eqsys->GetRunawayGrid();
    FVM::Grid *fluidGrid   = eqsys->GetFluidGrid();
    enum OptionConstants::momentumgrid_type ht_type = eqsys->GetHotTailGridType();
    enum OptionConstants::momentumgrid_type re_type = eqsys->GetRunawayGridType();

    // Fluid equations
    ConstructEquation_Ions(eqsys, s, adas);
    IonHandler *ionHandler = eqsys->GetIonHandler();
    // Construct collision quantity handlers
    FVM::UnknownQuantityHandler *unknowns = eqsys->GetUnknownHandler();
    if (hottailGrid != nullptr) {
        CollisionQuantityHandler *cqh = ConstructCollisionQuantityHandler(ht_type, hottailGrid, unknowns, ionHandler, s);
        eqsys->SetHotTailCollisionHandler(cqh);
    } else if (runawayGrid != nullptr) {
        CollisionQuantityHandler *cqh = ConstructCollisionQuantityHandler(re_type, runawayGrid, unknowns, ionHandler, s);
        eqsys->SetRunawayCollisionHandler(cqh);
    }
    RunawayFluid *REF = ConstructRunawayFluid(fluidGrid,unknowns,ionHandler,re_type,s);
    eqsys->SetREFluid(REF);

    // Post processing handler
    PostProcessor *postProcessor = new PostProcessor(fluidGrid, unknowns);
    eqsys->SetPostProcessor(postProcessor);

    // Hot-tail quantities
    if (eqsys->HasHotTailGrid()) {
        ConstructEquation_f_hot(eqsys, s);
    }

    // Runaway quantities
    if (eqsys->HasRunawayGrid()) {
        ConstructEquation_f_re(eqsys, s);
    }
    ConstructEquation_E_field(eqsys, s);
    ConstructEquation_j_hot(eqsys, s);
    ConstructEquation_j_tot(eqsys, s);
    ConstructEquation_j_ohm(eqsys, s);
    ConstructEquation_n_cold(eqsys, s);
    ConstructEquation_n_hot(eqsys, s);
    ConstructEquation_T_cold(eqsys, s, adas, nist);

    // NOTE: The runaway number may depend explicitly on
    // the hot-tail equation and must therefore be constructed
    // AFTER the call to 'ConstructEquation_f_hot()'
    ConstructEquation_n_re(eqsys, s);

    // Helper quantities
    ConstructEquation_n_tot(eqsys, s);

}

/**
 * Load initialization settings for the EquationSystem.
 *
 * eqsys:       Equation system to define quantities in.
 * s:           Settings object specifying how to construct
 *              the equation system.
 */
real_t SimulationGenerator::ConstructInitializer(
    EquationSystem *eqsys, Settings *s
) {
    real_t t0 = s->GetReal(INITIALIZATION "/t0");
    const string& filename = s->GetString(INITIALIZATION "/fromfile");
    int_t timeIndex = s->GetInteger(INITIALIZATION "/filetimeindex");

    // Initialize from previous output
    if (filename != "") {
        vector<string> ignoreList = s->GetStringList(INITIALIZATION "/eqsysignore");
        eqsys->SetInitializerFile(filename, ignoreList, timeIndex);
    }

    return t0;
}

/**
 * Construct the unknowns of the equation system.
 *
 * eqsys:       Equation system to define quantities in.
 * s:           Settings object specifying how to construct
 *              the equation system.
 * fluidGrid:   Radial grid for the computation.
 * hottailGrid: Grid on which the hot-tail electron population
 *              is computed.
 * runawayGrid: Grid on which the runaway electron population
 *              is computed.
 *
 * NOTE: The 'hottailGrid' and 'runawayGrid' will be 'nullptr'
 *       if disabled.
 */
void SimulationGenerator::ConstructUnknowns(
    EquationSystem *eqsys, Settings *s, FVM::Grid *scalarGrid, FVM::Grid *fluidGrid,
    FVM::Grid *hottailGrid, FVM::Grid *runawayGrid
) {
    // Hot-tail quantities
    if (hottailGrid != nullptr) {
        eqsys->SetUnknown(OptionConstants::UQTY_F_HOT, hottailGrid);
    }

    // Fluid quantities
    len_t nIonChargeStates = GetNumberOfIonChargeStates(s);
    eqsys->SetUnknown(OptionConstants::UQTY_ION_SPECIES, fluidGrid, nIonChargeStates);
    eqsys->SetUnknown(OptionConstants::UQTY_N_HOT, fluidGrid);
    eqsys->SetUnknown(OptionConstants::UQTY_N_COLD, fluidGrid);
    eqsys->SetUnknown(OptionConstants::UQTY_N_RE, fluidGrid);
    eqsys->SetUnknown(OptionConstants::UQTY_J_OHM, fluidGrid);
    eqsys->SetUnknown(OptionConstants::UQTY_J_HOT, fluidGrid);
    eqsys->SetUnknown(OptionConstants::UQTY_J_TOT, fluidGrid);
    eqsys->SetUnknown(OptionConstants::UQTY_T_COLD, fluidGrid);
    eqsys->SetUnknown(OptionConstants::UQTY_E_FIELD, fluidGrid);    
    eqsys->SetUnknown(OptionConstants::UQTY_POL_FLUX, fluidGrid);
    eqsys->SetUnknown(OptionConstants::UQTY_I_P, scalarGrid);
    eqsys->SetUnknown(OptionConstants::UQTY_PSI_EDGE, scalarGrid);

 
    // Fluid helper quantities
    eqsys->SetUnknown(OptionConstants::UQTY_N_TOT, fluidGrid);

    // Runaway quantities
    if (runawayGrid != nullptr) {
        eqsys->SetUnknown(OptionConstants::UQTY_F_RE, runawayGrid);
    }
}
