/**
 * Set up a collision quantity handler.
 */

#include <string>
#include "DREAM/Equations/CollisionQuantityHandler.hpp"
#include "DREAM/Settings/SimulationGenerator.hpp"


using namespace DREAM;
using namespace std;


#define MODNAME "collisions"

/**
 * Define options applying to collision models.
 *
 * name: Name of grid settings group to define options in.
 * s:    Settings object to define options in.
 */
void SimulationGenerator::DefineOptions_CollisionQuantityHandler(
    const std::string& mod, Settings *s
) {
    s->DefineSetting(mod + "/" MODNAME "/lnlambda", "Model to use when evaluating Coulomb logarithm", (int_t)OptionConstants::COLLQTY_LNLAMBDA_CONSTANT);
    s->DefineSetting(mod + "/" MODNAME "/collfreq_mode", "Mode in which to evaluate collision frequencies", (int_t)OptionConstants::COLLQTY_COLLISION_FREQUENCY_MODE_SUPERTHERMAL);
    s->DefineSetting(mod + "/" MODNAME "/collfreq_type", "Model to use when evaluating collision frequencies", (int_t)OptionConstants::COLLQTY_COLLISION_FREQUENCY_TYPE_NON_SCREENED);
}

/**
 * Construct a CollisionQuantityHandler object for the specified
 * grid object. Read settings from the named section of the settings.
 *
 * name:     Name of settings section to load options from.
 * grid:     Grid object for which to construct the collision handler.
 * unknowns: List of unknowns in the associated equation system.
 * s:        Settings describing how to construct the collision handler.
 */
CollisionQuantityHandler *SimulationGenerator::ConstructCollisionQuantityHandler(
    const string& /*name*/,
    enum OptionConstants::momentumgrid_type gridtype, FVM::Grid *grid,
    FVM::UnknownQuantityHandler *unknowns, Settings* /*s*/
) {
    struct CollisionQuantityHandler::collqtyhand_settings *cq =
        new CollisionQuantityHandler::collqtyhand_settings;

    CollisionQuantityHandler *cqh = new CollisionQuantityHandler(cq);
    cqh->SetGrid(grid, gridtype);
    cqh->SetUnknowns(unknowns);

    return cqh;
}

