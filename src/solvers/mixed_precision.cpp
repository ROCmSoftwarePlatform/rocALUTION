#include "../utils/def.hpp"
#include "../utils/types.hpp"
#include "mixed_precision.hpp"
#include "iter_ctrl.hpp"

#include "../base/local_matrix.hpp"

#include "../base/local_vector.hpp"

#include "../utils/log.hpp"
#include "../utils/allocate_free.hpp"

#include <math.h>

namespace rocalution {

template <class OperatorTypeH, class VectorTypeH, typename ValueTypeH,
          class OperatorTypeL, class VectorTypeL, typename ValueTypeL>
MixedPrecisionDC<OperatorTypeH, VectorTypeH, ValueTypeH, 
                 OperatorTypeL, VectorTypeL, ValueTypeL>::MixedPrecisionDC() {

  LOG_DEBUG(this, "MixedPrecisionDC::MixedPrecisionDC()",
            "default constructor");

  this->op_l_ = NULL;
  this->Solver_L_ = NULL;

}

template <class OperatorTypeH, class VectorTypeH, typename ValueTypeH,
          class OperatorTypeL, class VectorTypeL, typename ValueTypeL>
MixedPrecisionDC<OperatorTypeH, VectorTypeH, ValueTypeH, 
                 OperatorTypeL, VectorTypeL, ValueTypeL>::~MixedPrecisionDC() {

  LOG_DEBUG(this, "MixedPrecisionDC::~MixedPrecisionDC()",
            "destructor");

  this->Clear();

}

template <class OperatorTypeH, class VectorTypeH, typename ValueTypeH,
          class OperatorTypeL, class VectorTypeL, typename ValueTypeL>
void MixedPrecisionDC<OperatorTypeH, VectorTypeH, ValueTypeH, 
                      OperatorTypeL, VectorTypeL, ValueTypeL>::Set(Solver<OperatorTypeL, VectorTypeL, 
                                                                    ValueTypeL> &Solver_L) {

  LOG_DEBUG(this, "MixedPrecisionDC::Set()",
            "");

  this->Solver_L_ = &Solver_L;
  
}

template <class OperatorTypeH, class VectorTypeH, typename ValueTypeH,
          class OperatorTypeL, class VectorTypeL, typename ValueTypeL>
void MixedPrecisionDC<OperatorTypeH, VectorTypeH, ValueTypeH, 
                      OperatorTypeL, VectorTypeL, ValueTypeL>::Print(void) const {
  
  if (this->Solver_L_ == NULL) { 
    
    LOG_INFO("MixedPrecisionDC solver");
    
  } else {
    
    LOG_INFO("MixedPrecisionDC solver, with solver:");
    this->Solver_L_->Print();

  }

  
}


template <class OperatorTypeH, class VectorTypeH, typename ValueTypeH,
          class OperatorTypeL, class VectorTypeL, typename ValueTypeL>
void MixedPrecisionDC<OperatorTypeH, VectorTypeH, ValueTypeH, 
                      OperatorTypeL, VectorTypeL, ValueTypeL>::PrintStart_(void) const {

  assert(this->Solver_L_ != NULL);

  LOG_INFO("MixedPrecisionDC [" << 8*sizeof(ValueTypeH) << "bit-" << 8*sizeof(ValueTypeL) 
           << "bit] solver starts, with solver:");
  this->Solver_L_->Print();

}

template <class OperatorTypeH, class VectorTypeH, typename ValueTypeH,
          class OperatorTypeL, class VectorTypeL, typename ValueTypeL>
void MixedPrecisionDC<OperatorTypeH, VectorTypeH, ValueTypeH, 
                      OperatorTypeL, VectorTypeL, ValueTypeL>::PrintEnd_(void) const {

  LOG_INFO("MixedPrecisionDC ends");

}

template <class OperatorTypeH, class VectorTypeH, typename ValueTypeH,
          class OperatorTypeL, class VectorTypeL, typename ValueTypeL>
void MixedPrecisionDC<OperatorTypeH, VectorTypeH, ValueTypeH, 
                      OperatorTypeL, VectorTypeL, ValueTypeL>::Build(void) {

  LOG_DEBUG(this, "MixedPrecisionDC::Build()",
            this->build_ <<
            " #*# begin");

  if (this->build_ == true)
    this->Clear();

  assert(this->build_ == false);
  this->build_ = true;

  assert(this->Solver_L_ != NULL);
  assert(this->op_ != NULL);

  this->op_h_ = this->op_;

  assert(this->op_->get_nrow() == this->op_->get_ncol());
  assert(this->op_->get_nrow() > 0);

  assert(this->op_l_ == NULL);
  this->op_l_ = new OperatorTypeL;
        
  this->r_l_.Allocate("r_l", this->op_l_->get_nrow());  
  this->r_h_.Allocate("r_h", this->op_h_->get_nrow());

  this->d_h_.Allocate("d_h", this->op_h_->get_nrow());
  this->d_l_.Allocate("d_l", this->op_h_->get_nrow());
  
  // TODO - ugly
  // copy the matrix

  // CSR H
  int *row_offset = NULL;
  int *col = NULL;
  ValueTypeH *val_h = NULL;

  // CSR L
  ValueTypeL *val_l = NULL;

  allocate_host(this->op_h_->get_local_nrow()+1, &row_offset);
  allocate_host(this->op_h_->get_local_nnz(),    &col);
  allocate_host(this->op_h_->get_local_nnz(),    &val_l);
  allocate_host(this->op_h_->get_local_nnz(),    &val_h);
  
  this->op_h_->CopyToCSR(row_offset, col, val_h);

  for (IndexType2 i=0; i<this->op_h_->get_nnz(); ++i)
    val_l[i] = ValueTypeL( val_h[i] );
  
  this->op_l_->SetDataPtrCSR(&row_offset, &col, &val_l,
                             "Low prec Matrix", 
                             this->op_h_->get_local_nnz(),
                             this->op_h_->get_local_nrow(),
                             this->op_h_->get_local_ncol());
  
  // free only the h prec values
  free_host(&val_h);

  this->Solver_L_->SetOperator(*this->op_l_);
  this->Solver_L_->Build();

  this->op_l_->MoveToAccelerator();
  this->Solver_L_->MoveToAccelerator();

  LOG_DEBUG(this, "MixedPrecisionDC::Build()",
            this->build_ <<
            " #*# end");

}

template <class OperatorTypeH, class VectorTypeH, typename ValueTypeH,
          class OperatorTypeL, class VectorTypeL, typename ValueTypeL>
void MixedPrecisionDC<OperatorTypeH, VectorTypeH, ValueTypeH, 
                      OperatorTypeL, VectorTypeL, ValueTypeL>::ReBuildNumeric(void) {

  LOG_DEBUG(this, "MixedPrecisionDC::ReBuildNumeric()",
            this->build_);

  if (this->build_ == true) {

    this->r_l_.Zeros();
    this->r_h_.Zeros();

    this->d_l_.Zeros();
    this->d_h_.Zeros();
    
    this->iter_ctrl_.Clear();

    if (this->Solver_L_ != NULL) {
      this->Solver_L_->ReBuildNumeric();
    }

  } else {

    this->Build();

  }

}

template <class OperatorTypeH, class VectorTypeH, typename ValueTypeH,
          class OperatorTypeL, class VectorTypeL, typename ValueTypeL>
void MixedPrecisionDC<OperatorTypeH, VectorTypeH, ValueTypeH, 
                      OperatorTypeL, VectorTypeL, ValueTypeL>::Clear(void) {

  LOG_DEBUG(this, "MixedPrecisionDC::Clear()",
            this->build_);

  if (this->build_ == true) {

    if (this->Solver_L_ != NULL) {
      this->Solver_L_->Clear();
      this->Solver_L_   = NULL;
    }

    if (this->op_l_ != NULL) {
      delete this->op_l_;
      this->op_l_ = NULL;
    }

    this->r_l_.Clear();
    this->r_h_.Clear();

    this->d_l_.Clear();
    this->d_h_.Clear();
    
    this->iter_ctrl_.Clear();
    
    this->build_ = false;

  }

}


template <class OperatorTypeH, class VectorTypeH, typename ValueTypeH,
          class OperatorTypeL, class VectorTypeL, typename ValueTypeL>
void MixedPrecisionDC<OperatorTypeH, VectorTypeH, ValueTypeH, 
                      OperatorTypeL, VectorTypeL, ValueTypeL>::MoveToHostLocalData_(void) {

  if (this->build_ == true) {

    LOG_VERBOSE_INFO(2, "MixedPrecisionDC: the inner solver is always performed on the accel; this function does nothing");

  }

}

template <class OperatorTypeH, class VectorTypeH, typename ValueTypeH,
          class OperatorTypeL, class VectorTypeL, typename ValueTypeL>
void MixedPrecisionDC<OperatorTypeH, VectorTypeH, ValueTypeH, 
                      OperatorTypeL, VectorTypeL, ValueTypeL>::MoveToAcceleratorLocalData_(void) {

  if (this->build_ == true) {

    LOG_VERBOSE_INFO(2, "MixedPrecisionDC: the inner solver is always performed on the accel; this function does nothing");

  }

}

template <class OperatorTypeH, class VectorTypeH, typename ValueTypeH,
          class OperatorTypeL, class VectorTypeL, typename ValueTypeL>
void MixedPrecisionDC<OperatorTypeH, VectorTypeH, ValueTypeH, 
                      OperatorTypeL, VectorTypeL, ValueTypeL>::SolveNonPrecond_(const VectorTypeH &rhs,
                                                                                VectorTypeH *x) {

  LOG_DEBUG(this, "MixedPrecisionDC::SolveNonPrecond_()",
            " #*# begin");

  assert(x != NULL);
  assert(x != &rhs);
  assert(this->op_  != NULL);
  assert(this->Solver_L_  != NULL);
  assert(this->build_ == true);


  this->x_h_ = x;

  // initial residual = b - Ax
  this->op_h_->Apply(*this->x_h_, &this->r_h_); 
  this->r_h_.ScaleAdd(ValueTypeH(-1.0), rhs);

  ValueTypeH res = this->Norm(this->r_h_);

  if (this->iter_ctrl_.InitResidual(res) == false) {

    LOG_DEBUG(this, "MixedPrecisionDC::SolveNonPrecond_()",
              " #*# end");
    return;
  }

  while (!this->iter_ctrl_.CheckResidual(res, this->index_)) {

  // cast to lower precision 

  // TODO 
  // use template
  this->r_l_.CopyFromDouble(this->r_h_);

  this->r_l_.MoveToAccelerator();

  this->d_l_.Clear();
  this->d_l_.MoveToAccelerator();

  LOG_VERBOSE_INFO(2, "MixedPrecisionDC: starting the internal solver [" << 8*sizeof(ValueTypeL) << "bit]");

  // set the initial solution to zero
  this->d_l_.Allocate("d_l",this->r_l_.get_size());
  this->d_l_.Zeros();
  // solver the inner problem (low)
  this->Solver_L_->Solve(this->r_l_,
                         &this->d_l_);


  this->r_l_.Clear();
  this->r_l_.MoveToHost();
  this->d_l_.MoveToHost();

  LOG_VERBOSE_INFO(2, "MixedPrecisionDC: defect correcting on the host [" << 8*sizeof(ValueTypeH) << "bit]");

  // TODO 
  // use template
  this->d_h_.CopyFromFloat(this->d_l_);

  this->x_h_->AddScale(this->d_h_, ValueTypeH(1.0));

  // initial residual = b - Ax
  this->op_h_->Apply(*this->x_h_, &this->r_h_); 
  this->r_h_.ScaleAdd(ValueTypeH(-1.0), rhs);
  res = this->Norm(this->r_h_);

  }

  LOG_DEBUG(this, "MixedPrecisionDC::SolveNonPrecond_()",
            " #*# end");

}

template <class OperatorTypeH, class VectorTypeH, typename ValueTypeH,
          class OperatorTypeL, class VectorTypeL, typename ValueTypeL>
void MixedPrecisionDC<OperatorTypeH, VectorTypeH, ValueTypeH, 
                      OperatorTypeL, VectorTypeL, ValueTypeL>::SolvePrecond_(const VectorTypeH &rhs,
                                                                             VectorTypeH *x) {
  assert(x != NULL);
  assert(x != &rhs);
  assert(this->op_  != NULL);
  assert(this->build_ == true);

  LOG_INFO("MixedPrecisionDC solver does not work with preconditioner. Perhaps you want to set the preconditioner to the internal solver?");
  FATAL_ERROR(__FILE__, __LINE__);

}


template class MixedPrecisionDC< LocalMatrix<double>, LocalVector<double>, double,
                                 LocalMatrix<float>,  LocalVector<float>, float >;

}
