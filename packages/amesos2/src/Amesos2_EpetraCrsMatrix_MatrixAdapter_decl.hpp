/**
 * \file   Amesos2_EpetraCrsMatrix_MatrixAdapter_decl.hpp
 * \author Eric Bavier <etbavie@sandia.gov>
 * \date   Tue Jun 14 17:17:00 MDT 2011
 * 
 * \brief Specialization of the ConcreteMatrixAdapter for
 * Epetra_CrsMatrix.  Inherits all its functionality from the
 * Epetra_RowMatrix specialization of \c AbstractConcreteMatrixAdapter.
 */

#ifndef AMESOS2_EPETRACRSMATRIX_MATRIXADAPTER_DECL_HPP
#define AMESOS2_EPETRACRSMATRIX_MATRIXADAPTER_DECL_HPP

#include "Amesos2_config.h"

#include "Amesos2_EpetraRowMatrix_AbstractMatrixAdapter.hpp"
#include "Amesos2_MatrixAdapter_decl.hpp"

namespace Amesos {

  template <>
  class ConcreteMatrixAdapter< Epetra_CrsMatrix >
    : public AbstractConcreteMatrixAdapter< Epetra_RowMatrix, Epetra_CrsMatrix >
  {
    // Give our matrix adapter class access to our private
    // implementation functions
    friend class MatrixAdapter< Epetra_RowMatrix >;
  public:
    typedef Epetra_CrsMatrix                               matrix_t;
  private:
    typedef AbstractConcreteMatrixAdapter<Epetra_RowMatrix,
					  Epetra_CrsMatrix> super_t;
  public:
    // 'import' superclass types
    typedef super_t::scalar_t                     scalar_t;
    typedef super_t::local_ordinal_t       local_ordinal_t;
    typedef super_t::global_ordinal_t     global_ordinal_t;
    typedef super_t::node_t                         node_t;
    typedef super_t::global_size_t           global_size_t;
    
    typedef ConcreteMatrixAdapter<matrix_t>                    type;
    
    ConcreteMatrixAdapter(RCP<matrix_t> m);
    
    RCP<const MatrixAdapter<matrix_t> > get_impl(EDistribution d) const;
    
  };

} // end namespace Amesos

#endif	// AMESOS2_EPETRACRSMATRIX_MATRIXADAPTER_DECL_HPP
