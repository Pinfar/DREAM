#ifndef _DREAM_OPTION_CONSTANTS_HPP
#define _DREAM_OPTION_CONSTANTS_HPP

namespace DREAM {
    class OptionConstants {
    public:
        #include "OptionConstants.enum.hpp"

        // CONSTANTS
        // When adding a new constant here, remember
        // to also define it in 'src/Settings/Constants.cpp'.
        // Please, also maintain alphabetical order.
        static const char *UQTY_E_FIELD;
        static const char *UQTY_F_HOT;
        static const char *UQTY_F_RE;
        static const char *UQTY_ION_SPECIES;
        static const char *UQTY_I_WALL;
        static const char *UQTY_I_P;
        static const char *UQTY_J_HOT;
        static const char *UQTY_J_OHM;
        static const char *UQTY_J_RE;
        static const char *UQTY_J_TOT;
        static const char *UQTY_N_COLD;
        static const char *UQTY_N_HOT;
        static const char *UQTY_N_RE;
        static const char *UQTY_N_TOT;
        static const char *UQTY_POL_FLUX;
        static const char *UQTY_PSI_WALL;
        static const char *UQTY_PSI_EDGE;
        static const char *UQTY_S_PARTICLE;
        static const char *UQTY_T_COLD;
        static const char *UQTY_V_LOOP_WALL;
        static const char *UQTY_W_COLD;
    };
}

#endif/*_DREAM_OPTION_CONSTANTS_HPP*/
