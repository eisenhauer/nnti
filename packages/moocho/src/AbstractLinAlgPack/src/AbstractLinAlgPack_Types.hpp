// ///////////////////////////////////////////////////////////////
// AbstractLinAlgPackTypes.hpp
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

#ifndef ABSTRACT_LIN_ALG_PACK_TYPES_H
#define ABSTRACT_LIN_ALG_PACK_TYPES_H

#include <memory>
#include <stdexcept>

#include "RTOpPack/src/RTOp.h"
#include "BLAS_CppTypes.hpp"
#include "Range1D.hpp"
#include "DenseLinAlgPack/src/DenseLinAlgPackTypes.hpp"

namespace AbstractLinAlgPack {

#include "DenseLinAlgPack/src/DenseLinAlgPackPublicTypes.ud"

typedef RTOp_index_type  size_type;
typedef RTOp_value_type  value_type;
typedef RTOp_index_type  index_type;

typedef RangePack::Range1D Range1D; // For some reason doxygen likes typedef more than using?

/** @name Exception classes */
//@{

/// Base class for precondition exceptions
class PreConditionException : public std::logic_error
{public: PreConditionException(const std::string& what_arg) : std::logic_error(what_arg) {}};

/// Base class for postcondition exceptions
class PostConditionException : public std::runtime_error
{public: PostConditionException(const std::string& what_arg) : std::runtime_error(what_arg) {}};

/// Base class for input exceptions (Preconditions).
class InputException : public PreConditionException
{public: InputException(const std::string& what_arg) : PreConditionException(what_arg) {}};

/// Base class for invalid setup for a class object when an exception is thrown
class SetupException : public PreConditionException
{public: SetupException(const std::string& what_arg) : PreConditionException(what_arg) {}};


//@}

/** @name Main interface library */
//@{

class InnerProduct;

class VectorSpaceFactory;
class VectorSpace;
class Vector;
class VectorMutable;

class MatrixBase;
class MatrixOp;
class MatrixNonsing;
class MatrixOpNonsing;
class MatrixSymOp;
class MatrixSymNonsing;
class MatrixSymOpNonsing;
class MatrixSymDiag;

class MultiVector;
class MultiVectorMutable;

class MatrixSymSecant;

class BasisSystem;
class BasisSystemPerm;
class BasisSystemFactory;

class Permutation;

// template classes

template <class T_Indice, class T_Value>	class SparseElement;
template <class T_Element, class T_Alloc>	class SparseVector;
template <class T_Element>					class SparseVectorSlice;

// concrete classes

class EtaVector;
class GenPermMatrixSlice;
typedef SparseVector<
	SparseElement<index_type,value_type>
	, std::allocator<
		SparseElement<index_type,value_type>
	  >
  >												SpVector;
typedef SparseVectorSlice<
	SparseElement<index_type,value_type> >		SpVectorSlice;

//@}

/** @name Standard tools library */
//@{

// pure abstract classes

class PermVector;

class MatrixSymInitDiag;
class MatrixSymDiag;

// concrete subclasses

class BasisSystemComposite;
class VectorSpaceBlock;
class VectorMutableBlock;
class MatrixOpSubView;
class MatrixComposite;
class MatrixSymIdent;
class MatrixSymDiagStd;
class MatrixZero;
class MatrixPermAggr;
class MatrixOpNonsingAggr;

// testing classes

class VectorSpaceTester;
class VectorSpaceTesterSetOptions;
class MatrixOpNonsingTester;
class BasisSystemTester;
class BasisSystemTesterSetOptions;

//@}

/** @name Serial interface library */
//@{

// pure abstract classes

class MatrixWithOpSerial;
class MatrixNonsingularSerial;
class MatrixSymWithOpSerial;
class MatrixSymNonsingularSerial;
class MatrixWithOpNonsingularSerial;
class MatrixSymWithOpNonsingularSerial;
class MatrixSymDenseInitialize;
class MatrixSymDiagonalSparse;
class MatrixLoadSparseElements;
class MatrixConvertToSparse;
class MatrixExtractSparseElements;
class MatrixExtractInvCholFactor;
class MatrixSymWithOpGetGMSSymMutable;
class MatrixSymWithOpGetGMSSym;
class MatrixSymAddDelUpdateable;

//@}

/** @name Serial implementations library */
//@{

class VectorDenseEncap;
class VectorDenseMutableEncap;
class MatrixDenseEncap;
class MatrixDenseMutableEncap;
class MatrixDenseSymEncap;
class MatrixDenseSymMutableEncap;
class MatrixDenseTriEncap;

class PermutationSerial;
class VectorSpaceSerial;
class VectorWithOpMutableDense;
class VectorWithOpSparse;
class MatrixSparseCOORSerial;
class MatrixSymPosDefCholFactor;
class MatrixConvertToSparseEncap;
class MultiVectorMutableDense;

class MatrixSymDiagonalSparseStd;

//@}

} // end namespace AbstractLinAlgPack

#endif // ABSTRACT_LIN_ALG_PACK_TYPES_H
