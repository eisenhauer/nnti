// //////////////////////////////////////////////////////////////////////
// SparseCOOPtrElement.h
//
// Copyright (C) 2001 Roscoe Ainsworth Bartlett
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the "Artistic License" (see the web site
//   http://www.opensource.org/licenses/artistic-license.html).
// This license is spelled out in the file COPYING.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// above mentioned "Artistic License" for more details.

#ifndef SPARSE_COO_PTR_ELEMENT_H
#define SPARSE_COO_PTR_ELEMENT_H

#include "SparseLinAlgPackTypes.h"

namespace SparseLinAlgPack {

///
/** Sparse pointer element type for a COO matrix (val, ivect, jvect).
 *
 * This class abstracts a sparse element of a templated
 * type from a coordinate matrix. It
 * has a pointer to the value of the element.
 *
 * The default assignment operator and copy constructor
 * are allowed.
 */
template <class T_Index, class T_Value>
class SparseCOOPtrElement {
public:
	/** @name Public Typedefs. */
	//@{

	///
	typedef T_Value						value_type;
	///
	typedef T_Index						index_type;

	//@}

	/** @name Constructors */
	//@{

	/// Construct uninitialized (poiner to value set to zero) (#index() == 0#).
	SparseCOOPtrElement() : pvalue_(0), row_i_(0), col_j_(0)
	{}

	/// Construct with a pointer to the value and index set
	SparseCOOPtrElement(value_type* pvalue, index_type row_i, index_type col_j)
		: pvalue_(pvalue), row_i_(row_i), col_j_(col_j)
	{}

	/// Initialize
	void initialize(value_type* pvalue, index_type row_i, index_type col_j) {
		pvalue_	= pvalue;
		row_i_	= row_i;
		col_j_	= col_j;
	}
	
	//@}

	/** @name Value and index access */
	//@{ 

	///
	value_type& value()
	{
		return *pvalue_;
	}
	///
	value_type value() const
	{
		return *pvalue_;
	}
	///
	index_type row_i() const
	{
		return row_i_;
	}
	///
	index_type col_j() const
	{
		return col_j_;
	}
	/// Change the indexs
	void change_indexes(index_type row_i, index_type col_j)
	{
		row_i_ = row_i;
		col_j_ = col_j;
	}
	/// Change the element pointer
	void change_value_ptr(value_type* pvalue)
	{
		pvalue_ = pvalue;
	}

	//@}
private:
	value_type*				pvalue_;
	index_type				row_i_, col_j_;

};	// end class SparseCOOPtrElement

} // end namespace SparseLinAlgPack 

#endif // SPARSE_COO_PTR_ELEMENT_H
