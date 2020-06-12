/**
 * Implementation of a base class for equation terms with diagonal
 * matrices. 
 * Evaluation of weights must be implemented in derived classes. 
 */

#include "FVM/Equation/Equation.hpp"
#include "FVM/Equation/DiagonalTerm.hpp"
#include "FVM/Grid/Grid.hpp"


using namespace DREAM::FVM;


/**
 * Constructor.
 */
DiagonalTerm::DiagonalTerm(Grid *g) : EquationTerm(g){}

/**
 * Destructor
 */
DiagonalTerm::~DiagonalTerm() {
    this->DeallocateWeights();
}


/**
 * Allocate and set weights.  
 */
void DiagonalTerm::InitializeWeights(){
    AllocateWeights(); 
    SetWeights();
    hasBeenInitialized = false;
}

/**
 * If grid has been rebuilt, initialize weights.
 * Otherwise, if weights are unknown-dependent, 
 * rebuild weights. Otherwise, do nothing.
 */
void DiagonalTerm::Rebuild(const real_t, const real_t, UnknownQuantityHandler*){ 
    if(!hasBeenInitialized){
        InitializeWeights();
        hasBeenInitialized = true;
    } else if(TermDependsOnUnknowns()) 
        SetWeights();
}


/**
 * Call when the grid is rebuilt; reallocates grid-quantities
 * and marks weights for reinitialisation.
 */
bool DiagonalTerm::GridRebuilt(){
    this->AllocateMemory();
    hasBeenInitialized = false;

    return true;
}

/**
 * Set a block for this term in the given jacobian matrix.
 */
void DiagonalTerm::SetJacobianBlock(
    const len_t uqtyId, const len_t derivId, Matrix *jac, const real_t* x
) {
    if (derivId == uqtyId) {
        this->SetMatrixElements(jac, nullptr);
    }
    AddWeightsJacobian(uqtyId, derivId, jac, x);
}

/**
 * Allocate weights
 */
void DiagonalTerm::AllocateWeights(){
    DeallocateWeights(); 

    weights = new real_t[GetNumberOfWeightsElements()];
}

/**
 * Deallocate weights
 */
void DiagonalTerm::DeallocateWeights(){
    if(weights!=nullptr) 
    delete[] weights;
}


