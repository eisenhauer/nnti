#include "ILUKPreconditionerFactory.h"

#include "TSFVectorSpaceBase.h"
#include "TSFVectorBase.h"
#include "TSFMatrixOperator.h"
#include "TSFUtils.h"
#include "TSFLinearSolverBase.h"

using namespace TSF;
using std::string;

ILUKPreconditionerFactory::ILUKPreconditionerFactory(int fillLevels,
																										 int overlapFill)
	: TSFPreconditionerFactoryBase(), fillLevels_(fillLevels),
		overlapFill_(overlapFill)
{}

TSFPreconditioner ILUKPreconditionerFactory
::createPreconditioner(const TSFLinearOperator& A) const
{
	TSFPreconditioner rtn;

	if (A.isMatrixOperator())
		{
			const TSFSmartPtr<const TSFMatrixOperator> matrix = A.getMatrix();
			matrix->getILUKPreconditioner(fillLevels_, overlapFill_, rtn);
		}
	else
		{
			TSFError::raise("ILUKPreconditionerFactory::createPreconditioner called "
											"for a matrix-free operator");
		}
	return rtn;
}

/* VEH createPreconditioner method using a TSFOperatorSource factory */
/* Not yet implemented */
TSFPreconditioner ILUKPreconditionerFactory
::createPreconditioner(const TSFOperatorSource& S) const
{
  TSFError::raise("ILUKPreconditionerFactory::createPreconditioner called "
                  "for a TSFOperatorSource. Not implemented yet.");
}


string ILUKPreconditionerFactory::toString() const 
{
	return "ILUK factory (fill level=" + TSFUtils::toString(fillLevels_)
		+ ")";
}
