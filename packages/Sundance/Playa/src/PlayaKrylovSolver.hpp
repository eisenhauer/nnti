/* @HEADER@ */
//   
 /* @HEADER@ */

#ifndef PLAYA_KRYLOVSOLVER_HPP
#define PLAYA_KRYLOVSOLVER_HPP

#include "PlayaDefs.hpp"
#include "PlayaIterativeSolver.hpp"
#include "PlayaPreconditionerFactory.hpp"
#include "PlayaILUKPreconditionerFactory.hpp"
#include "PlayaSimpleComposedOpDecl.hpp"

namespace Playa
{
using namespace Teuchos;

/**
 *
 */
template <class Scalar>
class KrylovSolver : public IterativeSolver<Scalar>
{
public:
  /** */
  KrylovSolver(const ParameterList& params);
  /** */
  KrylovSolver(const ParameterList& params,
    const PreconditionerFactory<Scalar>& precond);

  /** */
  virtual ~KrylovSolver(){;}

  /** */
  virtual SolverState<Scalar> solve(const LinearOperator<Scalar>& op,
    const Vector<Scalar>& rhs,
    Vector<Scalar>& soln) const ;
protected:
  virtual SolverState<Scalar> solveUnprec(const LinearOperator<Scalar>& op,
    const Vector<Scalar>& rhs,
    Vector<Scalar>& soln) const = 0 ;

  const PreconditionerFactory<Scalar>& precond() const {return precond_;}

private:
  PreconditionerFactory<Scalar> precond_;
};

  
template <class Scalar> inline
KrylovSolver<Scalar>::KrylovSolver(const ParameterList& params)
  : IterativeSolver<Scalar>(params), precond_()
{
  if (!params.isParameter("Precond")) return;

  const std::string& precondType = params.template get<string>("Precond");

  if (precondType=="ILUK")
  {
    precond_ = new ILUKPreconditionerFactory<Scalar>(params);
  }
}

template <class Scalar> inline
KrylovSolver<Scalar>::KrylovSolver(const ParameterList& params,
  const PreconditionerFactory<Scalar>& precond)
  : IterativeSolver<Scalar>(params), precond_(precond)
{
  TEUCHOS_TEST_FOR_EXCEPTION(params.isParameter("Precond"), std::runtime_error,
    "ambiguous preconditioner specification in "
    "KrylovSolver ctor: parameters specify "
    << params.template get<string>("Precond") 
    << " but preconditioner argument is " 
    << precond);
}

template <class Scalar> inline
SolverState<Scalar> KrylovSolver<Scalar>
::solve(const LinearOperator<Scalar>& op,
  const Vector<Scalar>& rhs,
  Vector<Scalar>& soln) const
{
  if (precond_.ptr().get()==0) 
  {
    return solveUnprec(op, rhs, soln);
  }


  Preconditioner<Scalar> p = precond_.createPreconditioner(op);
    
  if (!p.hasRight())
  {
    LinearOperator<Scalar> A = p.left()*op;
    Vector<Scalar> newRHS = rhs.space().createMember();
    p.left().apply(rhs, newRHS);
    return solveUnprec(A, newRHS, soln);
  }
  else if (!p.hasLeft())
  {
    LinearOperator<Scalar> A = op * p.right();
    Vector<Scalar> intermediateSoln;
    SolverState<Scalar> rtn 
      = solveUnprec(A, rhs, intermediateSoln);
    if (rtn.finalState()==SolveConverged) 
    {
      p.right().apply(intermediateSoln, soln);
    }
    return rtn;
  }
  else
  {
    LinearOperator<Scalar> A = p.left() * op * p.right();
    Vector<Scalar> newRHS;
    p.left().apply(rhs, newRHS);
    Vector<Scalar> intermediateSoln;
    SolverState<Scalar> rtn 
      = solveUnprec(A, newRHS, intermediateSoln);
    if (rtn.finalState()==SolveConverged) 
    {
      p.right().apply(intermediateSoln, soln);
    }
    return rtn;
  }
}
  
}

#endif

