#ifndef _TQS_FVM_ADVECTION_TERM_HPP
#define _TQS_FVM_ADVECTION_TERM_HPP

#include "FVM/Equation/EquationTerm.hpp"
#include "FVM/Grid/RadialGrid.hpp"

namespace TQS::FVM {
    class AdvectionTerm : public EquationTerm {
    protected:
        len_t nr, n1, n2;
        real_t **fr=nullptr, **f1=nullptr, **f2=nullptr;
        bool coefficientsShared = false;

        void setMatrixElementsForCoordinate(
            Matrix*, const len_t offs, const len_t np1, const len_t np2,
            const real_t *F, const real_t *h1, const real_t *h2,
            const real_t *dx
        );

    public:
        AdvectionTerm(RadialGrid*);
        ~AdvectionTerm();

        void AllocateCoefficients();
        void DeallocateCoefficients();
        void SetCoefficients(real_t**, real_t**, real_t**);

        // Accessors to advection coefficients
        real_t& Fr(const len_t ir, const len_t i1, const len_t i2)
        { return fr[ir][i2*n1 + i1]; }
        real_t& F1(const len_t ir, const len_t i1, const len_t i2)
        { return f1[ir][i2*(n1+1) + i1]; }
        real_t& F2(const len_t ir, const len_t i1, const len_t i2)
        { return f2[ir][i2*n1 + i1]; }

        virtual bool GridRebuilt() override;
        virtual void SetMatrixElements(Matrix*) override;
    };
}

#endif/*_TQS_FVM_ADVECTION_TERM_HPP*/
