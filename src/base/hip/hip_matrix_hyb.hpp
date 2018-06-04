#ifndef ROCALUTION_HIP_MATRIX_HYB_HPP_
#define ROCALUTION_HIP_MATRIX_HYB_HPP_

#include "../base_matrix.hpp"
#include "../base_vector.hpp"
#include "../matrix_formats.hpp"

#include <hipsparse.h>

namespace rocalution {

template <typename ValueType>
class HIPAcceleratorMatrixHYB : public HIPAcceleratorMatrix<ValueType> {

public:

  HIPAcceleratorMatrixHYB();
  HIPAcceleratorMatrixHYB(const Rocalution_Backend_Descriptor local_backend);
  virtual ~HIPAcceleratorMatrixHYB();

  inline int get_ell_max_row(void) const { return this->mat_.ELL.max_row; }
  inline int get_ell_nnz(void) const { return this->ell_nnz_; }
  inline int get_coo_nnz(void) const { return this->coo_nnz_; }

  virtual void info(void) const;
  virtual unsigned int get_mat_format(void) const { return HYB; }

  virtual void Clear(void);
  virtual void AllocateHYB(const int ell_nnz, const int coo_nnz, const int ell_max_row,
                           const int nrow, const int ncol);

  virtual bool ConvertFrom(const BaseMatrix<ValueType> &mat);

  virtual void CopyFrom(const BaseMatrix<ValueType> &mat);
  virtual void CopyFromAsync(const BaseMatrix<ValueType> &mat);
  virtual void CopyTo(BaseMatrix<ValueType> *mat) const;
  virtual void CopyToAsync(BaseMatrix<ValueType> *mat) const;

  virtual void CopyFromHost(const HostMatrix<ValueType> &src);
  virtual void CopyFromHostAsync(const HostMatrix<ValueType> &src);
  virtual void CopyToHost(HostMatrix<ValueType> *dst) const;
  virtual void CopyToHostAsync(HostMatrix<ValueType> *dst) const;

  virtual void Apply(const BaseVector<ValueType> &in, BaseVector<ValueType> *out) const;
  virtual void ApplyAdd(const BaseVector<ValueType> &in, const ValueType scalar,
                        BaseVector<ValueType> *out) const;

private:

  MatrixHYB<ValueType, int> mat_;

  int ell_nnz_;
  int coo_nnz_;

  hipsparseMatDescr_t ell_mat_descr_;
  hipsparseMatDescr_t coo_mat_descr_;

  friend class BaseVector<ValueType>;
  friend class AcceleratorVector<ValueType>;
  friend class HIPAcceleratorVector<ValueType>;

};


}

#endif // ROCALUTION_HIP_MATRIX_HYB_HPP_