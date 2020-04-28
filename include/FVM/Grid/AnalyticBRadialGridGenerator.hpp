#ifndef _DREAM_FVM_GRID_ANALYTIC_B_RADIAL_GRID_GENERATOR_HPP
#define _DREAM_FVM_GRID_ANALYTIC_B_RADIAL_GRID_GENERATOR_HPP

#include "FVM/Grid/Grid.hpp"
#include "FVM/Grid/MomentumGrid.hpp"
#include "FVM/Grid/RadialGrid.hpp"
#include "FVM/Grid/RadialGridGenerator.hpp"
#include <functional>
#include <gsl/gsl_spline.h>

namespace DREAM::FVM {
    class AnalyticBRadialGridGenerator : public RadialGridGenerator {
    private:
        real_t rMin, rMax, R0;
        len_t nrProfiles;
        real_t *rProfilesProvided, *GsProvided, *psisProvided, 
            *kappasProvided, *deltasProvided, *DeltasProvided;
        real_t *G = nullptr, *psi, *kappa, *delta, *Delta,
            *GPrime, *psiPrime, *kappaPrime, *deltaPrime, *DeltaPrime;
        real_t *G_f, *psi_f, *kappa_f, *delta_f, *Delta_f,
            *GPrime_f, *psiPrime_f, *kappaPrime_f, *deltaPrime_f, *DeltaPrime_f;
        

        // Set to true when the grid is constructed for the first time
        bool isBuilt = false;
        real_t diffFunc(real_t r, std::function<real_t(real_t)> F); // = dF/dr at r

        void InterpolateInputProfileToGrid(real_t*, real_t*, real_t*&,real_t*&, real_t*&, real_t*&,real_t*);
        gsl_spline *spline_x;
        gsl_interp_accel *gsl_acc;

        
    public:
        AnalyticBRadialGridGenerator(const len_t nr,  real_t r0, 
             real_t ra,  real_t R0, len_t ntheta_ref, len_t ntheta_interp,
             real_t *, len_t , real_t *Gs, real_t *psi_p0s,
             real_t *kappas, real_t *deltas, real_t *Deltas);
        ~AnalyticBRadialGridGenerator();

        virtual bool NeedsRebuild(const real_t) const override { return (!isBuilt); }
        virtual bool Rebuild(const real_t, RadialGrid*) override;
        virtual void CreateMagneticFieldData(const real_t*, const real_t*) override;
        virtual void DeallocateShapeProfiles();
        

        
    };
}

#endif/*_DREAM_FVM_GRID_ANALYTIC_B_RADIAL_GRID_GENERATOR_HPP*/
