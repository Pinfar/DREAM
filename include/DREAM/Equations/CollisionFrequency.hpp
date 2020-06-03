#ifndef _DREAM_EQUATIONS_COLLISION_FREQUENCY_HPP
#define _DREAM_EQUATIONS_COLLISION_FREQUENCY_HPP

#include "CollisionQuantity.hpp"
#include "DREAM/Equations/CoulombLogarithm.hpp"

namespace DREAM {
    class CollisionFrequency : public CollisionQuantity {
    private:
        void InitializeGSLWorkspace();
        void DeallocateGSL();

        void SetPartialContributions(FVM::fluxGridType fluxGridType);

    protected:
        bool hasIonTerm;

        real_t **nonlinearMat = nullptr;
        real_t *trapzWeights = nullptr;
        
        CoulombLogarithm *lnLambdaEE;
        CoulombLogarithm *lnLambdaEI;
        
        real_t *nbound = nullptr;
        real_t **ionDensities;
        real_t *Zs;
        real_t **ionIndex;

        real_t *preFactor     = nullptr;
        real_t *preFactor_fr  = nullptr;
        real_t *preFactor_f1  = nullptr;
        real_t *preFactor_f2  = nullptr;
        virtual real_t evaluatePreFactorAtP(real_t p, OptionConstants::collqty_collfreq_mode collfreq_mode) = 0; 

        real_t *screenedTerm        = nullptr;
        real_t *screenedTerm_fr     = nullptr;
        real_t *screenedTerm_f1     = nullptr;
        real_t *screenedTerm_f2     = nullptr;
        virtual real_t evaluateScreenedTermAtP(len_t iz, len_t Z0, real_t p) = 0;
        
        real_t *ionTerm = nullptr;
        real_t *ionTerm_fr = nullptr;
        real_t *ionTerm_f1 = nullptr;
        real_t *ionTerm_f2 = nullptr;
        virtual real_t evaluateIonTermAtP(len_t iz, len_t Z0, real_t p) = 0;

        real_t **nColdTerm    = nullptr;
        real_t **nColdTerm_fr = nullptr;
        real_t **nColdTerm_f1 = nullptr;
        real_t **nColdTerm_f2 = nullptr;
        virtual real_t evaluateElectronTermAtP(len_t ir, real_t p, OptionConstants::collqty_collfreq_mode collfreq_mode) = 0;
        
        real_t *bremsTerm = nullptr;
        real_t *bremsTerm_fr = nullptr;
        real_t *bremsTerm_f1 = nullptr;
        real_t *bremsTerm_f2 = nullptr;
        virtual real_t evaluateBremsstrahlungTermAtP(len_t iz, len_t Z0, real_t p, OptionConstants::eqterm_bremsstrahlung_mode brems_mode, OptionConstants::collqty_collfreq_type collfreq_type) = 0;


        real_t *ionPartialContribution = nullptr;
        real_t *ionPartialContribution_fr = nullptr;
        real_t *ionPartialContribution_f1 = nullptr;
        real_t *ionPartialContribution_f2 = nullptr;

        real_t *nColdPartialContribution = nullptr;
        real_t *nColdPartialContribution_fr = nullptr;
        real_t *nColdPartialContribution_f1 = nullptr;
        real_t *nColdPartialContribution_f2 = nullptr;

        real_t *fHotPartialContribution_f1 = nullptr;

        real_t *atomicParameter = nullptr; // size nzs. Constant term
        //len_t *Zs = nullptr;


        static real_t psi0Integrand(real_t s, void *params);
        static real_t psi1Integrand(real_t s, void *params);
        virtual real_t evaluatePsi0(len_t ir, real_t p);
        virtual real_t evaluatePsi1(len_t ir, real_t p);
        virtual real_t evaluateExp1OverThetaK(real_t Theta, real_t n);
        
        gsl_integration_fixed_workspace **gsl_w = nullptr;
        gsl_integration_workspace *gsl_ad_w = nullptr;

        void setPreFactor(real_t *&preFactor, const real_t *pIn, len_t np1, len_t np2);
        void setElectronTerm(real_t **&nColdTerm, const real_t *pIn, len_t nr, len_t np1, len_t np2);
        void setScreenedTerm(real_t *&screenedTerm, const real_t *pIn, len_t np1, len_t np2);
        void setIonTerm(real_t *&ionTerm, const real_t *pIn, len_t np1, len_t np2);
        void setBremsTerm(real_t *&bremsTerm, const real_t *pIn, len_t np1, len_t np2);

        virtual void RebuildPlasmaDependentTerms() override;
        virtual void RebuildConstantTerms() override;
        virtual real_t GetAtomicParameter(len_t iz, len_t Z0) = 0;

        void SetNColdPartialContribution(real_t **nColdTerm,real_t *preFactor, real_t *const* lnLee, len_t nr, len_t np1, len_t np2, real_t *&partQty);
        void SetNiPartialContribution(real_t **nColdTerm, real_t *ionTerm, real_t *screenedTerm, real_t *bremsTerm, real_t *preFactor, real_t *const* lnLee,  real_t *const* lnLei, len_t nr, len_t np1, len_t np2, real_t *&partQty);
        void SetNonlinearPartialContribution(CoulombLogarithm *lnLambda, real_t *&partQty);
    
        



        virtual void calculateIsotropicNonlinearOperatorMatrix() = 0;
        
        virtual void AllocatePartialQuantities() override;
        void DeallocatePartialQuantities();

        void AllocateRadialQuantities();
        void DeallocateRadialQuantities();

        virtual void AssembleQuantity(real_t **&collisionQuantity, len_t nr, len_t np1, len_t np2, FVM::fluxGridType) override;

        const real_t* GetNColdPartialContribution(FVM::fluxGridType) const;
        const real_t* GetNiPartialContribution(FVM::fluxGridType)const;
        const real_t* GetNonlinearPartialContribution(FVM::fluxGridType)const;
    public:
        CollisionFrequency(FVM::Grid *g, FVM::UnknownQuantityHandler *u, IonHandler *ih,  
                CoulombLogarithm *lnLee,CoulombLogarithm *lnLei,
                enum OptionConstants::momentumgrid_type mgtype,  struct collqty_settings *cqset);
        virtual ~CollisionFrequency();

        void RebuildRadialTerms();

        void AddNonlinearContribution();
        //virtual real_t evaluateAtP()=0;
        const real_t *GetUnknownPartialContribution(len_t id_unknown,FVM::fluxGridType) const;
        virtual real_t evaluateAtP(len_t ir, real_t p) override;
        virtual real_t evaluateAtP(len_t ir, real_t p, struct collqty_settings *inSettings) override;


    };
}

#endif/*_DREAM_EQUATIONS_COLLISION_FREQUENCY_HPP*/
