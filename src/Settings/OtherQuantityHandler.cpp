/**
 * Construct an 'OtherQuantityHandler' object.
 */

#include <string>
#include "DREAM/OtherQuantityHandler.hpp"
#include "DREAM/Settings/SimulationGenerator.hpp"
#include "DREAM/Settings/Settings.hpp"


#define MODULENAME "other"

using namespace DREAM;
using namespace std;

/**
 * Define all options available for the 'OtherQuantityHandler'.
 */
void SimulationGenerator::DefineOptions_OtherQuantities(Settings *s) {
    s->DefineSetting(MODULENAME "/include", "List of names of other quantities to include", (const string)"");
}

/**
 * Construct an 'OtherQuantityHandler' object.
 */
void SimulationGenerator::ConstructOtherQuantityHandler(
    EquationSystem *eqsys, Settings *s
) {
    OtherQuantityHandler *oqh = new OtherQuantityHandler(
        eqsys->GetHotTailCollisionHandler(), eqsys->GetRunawayCollisionHandler(),
        eqsys->GetFluidGrid(), eqsys->GetHotTailGrid(), eqsys->GetRunawayGrid()
    );

    const vector<string> other = s->GetStringList(MODULENAME "/include");

    if (other.size() == 1 && other[0] == "all")
        oqh->RegisterAllQuantities();
    else {
        for (auto it = other.begin(); it != other.end(); it++)
            oqh->RegisterQuantity(*it);
    }

    eqsys->SetOtherQuantityHandler(oqh);
}

