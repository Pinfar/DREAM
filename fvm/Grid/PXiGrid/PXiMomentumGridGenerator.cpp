/**
 * Implementation of the p/xi grid generator.
 */

#include <cmath>
#include "FVM/config.h"
#include "FVM/Grid/MomentumGrid.hpp"
#include "FVM/Grid/PXiGrid/PXiMomentumGridGenerator.hpp"
#include "FVM/Grid/PXiGrid/PGridGenerator.hpp"
#include "FVM/Grid/PXiGrid/XiGridGenerator.hpp"
#include "FVM/Grid/RadialGrid.hpp"


using namespace DREAM::FVM::PXiGrid;


/**
 * Destructor.
 */
MomentumGridGenerator::~MomentumGridGenerator() {
    delete this->pGenerator;
    delete this->xiGenerator;
}

/**
 * Returns true if this momentum grid needs to
 * be re-built.
 *
 * t:            For which the grid should be re-built.
 * rGridRebuilt: True if the radial grid associated with
 *               with this generator was re-built for this
 *               time step.
 */
bool MomentumGridGenerator::NeedsRebuild(
    const real_t t, const bool rGridRebuilt
) {
    return (
        this->pGenerator->NeedsRebuild(t, rGridRebuilt) ||
        this->xiGenerator->NeedsRebuild(t, rGridRebuilt)
    );
}

/**
 * Re-builds the given momentum grid using this
 * momentum grid generator.
 */
bool MomentumGridGenerator::Rebuild(
    const real_t t, const len_t ri, DREAM::FVM::MomentumGrid *mg,
    const DREAM::FVM::RadialGrid *rg
) {
    bool built = this->pGenerator->Rebuild(t, ri, mg, rg);
    built |= this->xiGenerator->Rebuild(t, ri, mg, rg);

    return built;

}

