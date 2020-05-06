/**
 * Implementation of an equation term allowing for prescribed ion densities.
 */

#include "DREAM/Equations/Fluid/IonPrescribedParameter.hpp"
#include "DREAM/IonInterpolator1D.hpp"
#include "FVM/Grid/Grid.hpp"


using namespace DREAM;
using namespace std;


/**
 * Constructor.
 */
IonPrescribedParameter::IonPrescribedParameter(
    FVM::Grid *grid, IonHandler *ihdl, const len_t nIons,
    const len_t *ionIndices, IonInterpolator1D *data
) : EquationTerm(grid), ions(ihdl), nIons(nIons), ionIndices(ionIndices), iondata(data) {

    this->Z = new len_t[nIons];
    for (len_t i = 0; i < nIons; i++)
        this->Z[i] = this->ions->GetZ(ionIndices[i]);

    AllocateData();
}


/**
 * Destructor.
 */
IonPrescribedParameter::~IonPrescribedParameter() {
    DeallocateData();

    delete [] ionIndices;
    delete [] Z;
    delete [] iondata;
}


/**
 * Allocate memory for the 'currentData' variable, which
 * stores the interpolated ion densities.
 */
void IonPrescribedParameter::AllocateData() {
    const len_t Nr = this->grid->GetNr();
    len_t nChargeStates = 0;
    for (len_t i = 0; i < nIons; i++)
        nChargeStates += Z[i] + 1;

    this->currentData = new real_t*[nIons];
    this->currentData[0] = new real_t[nIons*nChargeStates*Nr];

    for (len_t i = 1; i < nIons; i++)
        this->currentData[i] = this->currentData[i-1] + (Z[i]+1);
}

/**
 * Deallocate memory for the 'currentData' variable.
 */
void IonPrescribedParameter::DeallocateData() {
    delete [] this->currentData[0];
    delete [] this->currentData;
}

/**
 * Rebuild this term for the given time. This routine will evaluate
 * the interpolators to obtain interpolated density data.
 *
 * t:        Time for which to evaluate the densities.
 * dt:       Time step to take (unused).
 * unknowns: List of unknowns (unused).
 */
void IonPrescribedParameter::Rebuild(const real_t t, const real_t, FVM::UnknownQuantityHandler*) {
    // If time has not passed, there's no need to rebuild
    // this term again
    if (t == lasttime)
        return;

    const len_t Nr = this->grid->GetNr();
    
    for (len_t i = 0, ionOffset = 0; i < nIons; i++) {
        for (len_t Z0 = 0; Z0 <= Z[i]; Z0++,ionOffset++) {
            const real_t *n = iondata->Eval(ionOffset, t);
            real_t *cd = currentData[i] + Z0*Nr;

            for (len_t ir = 0; ir < Nr; ir++)
                cd[ir] = n[ir];

            
        }
    }
}

/**
 * Sets the Jacobian matrix for the specified block
 * in the given matrix.
 *
 * uqtyId:  ID of the unknown quantity which the term
 *          is applied to (block row).
 * derivId: ID of the quantity with respect to which the
 *          derivative is to be evaluated.
 * mat:     Jacobian matrix block to populate.
 *
 * (This term represents a constant, and since the derivative
 * with respect to anything of a constant is zero, we don't need
 * to do anything).
 */
void IonPrescribedParameter::SetJacobianBlock(const len_t, const len_t, FVM::Matrix *jac) {
    const len_t Nr = this->grid->GetNr();

    for (len_t i = 0; i < nIons; i++) {
        for (len_t Z0 = 0; Z0 <= Z[i]; Z0++) {
            const len_t idx = this->ions->GetIndex(ionIndices[i], Z0);
            for (len_t ir = 0; ir < Nr; ir++)
                jac->SetElement(idx*Nr+ir, idx*Nr+ir, 1.0);
        }
    }
}

/**
 * Set the elements in the matrix and on the RHS corresponding
 * to this quantity.
 *
 * mat: Matrix to set elements in (1 is added to the diagonal)
 * rhs: Right-hand-side. Values will be set to the current value of
 *      this parameter.
 */
void IonPrescribedParameter::SetMatrixElements(FVM::Matrix *mat, real_t *rhs) {
    const len_t Nr = this->grid->GetNr();

    for (len_t i = 0; i < nIons; i++) {

        for (len_t Z0 = 0; Z0 <= Z[i]; Z0++) {
            const len_t idx = this->ions->GetIndex(ionIndices[i], Z0);
            real_t *n = currentData[i] + Z0*Nr;

            for (len_t ir = 0; ir < Nr; ir++)
                mat->SetElement(idx*Nr+ir, idx*Nr+ir, 1.0);
            for (len_t ir = 0; ir < Nr; ir++)
                rhs[idx*Nr+ir] += n[ir];
        }
    }
}

/**
 * Set the elements in the function vector 'F' (i.e.
 * evaluate this term).
 *
 * vec: Vector containing value of 'F' on return.
 * ni:  Ion densities in previous iteration.
 */
void IonPrescribedParameter::SetVectorElements(real_t *vec, const real_t *ni) {
    const len_t Nr = this->grid->GetNr();

    for (len_t i = 0; i < nIons; i++) {
        for (len_t Z0 = 0; Z0 <= Z[i]; Z0++) {
            const len_t idx = this->ions->GetIndex(ionIndices[i], Z0);
            real_t *n = currentData[i] + Z0*Nr;

            for (len_t ir = 0; ir < Nr; ir++)
                vec[idx*Nr+ir] += ni[idx*Nr+ir] - n[ir];
        }
    }
}

