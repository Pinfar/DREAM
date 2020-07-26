#ifndef _DREAM_EQUATIONS_CONNOR_HASTIE_HPP
#define _DREAM_EQUATIONS_CONNOR_HASTIE_HPP

namespace DREAM { class ConnorHastie; }

#include "DREAM/Equations/RunawayFluid.hpp"
#include "FVM/config.h"

namespace DREAM {
    class ConnorHastie {
    private:
        RunawayFluid *REFluid;

    public:
        ConnorHastie(RunawayFluid*);

        real_t RunawayRate(const len_t, const real_t, const real_t, const real_t, bool deriv=false);
        real_t Diff_EED(const len_t, const real_t, const real_t, const real_t);
        real_t Diff_E(const len_t, const real_t, const real_t, const real_t);
        real_t Diff_ne(const len_t, const real_t, const real_t, const real_t);
        real_t Diff_Te(const len_t, const real_t, const real_t, const real_t, const real_t);
    };
}

#endif/*_DREAM_EQUATIONS_CONNOR_HASTIE_HPP*/
