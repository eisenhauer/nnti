#! /usr/bin/env python

# @HEADER
# ************************************************************************
# 
#                PyTrilinos: Python Interface to Trilinos
#                   Copyright (2005) Sandia Corporation
# 
# Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
# license for use of this work by or on behalf of the U.S. Government.
# 
# This library is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 2.1 of the
# License, or (at your option) any later version.
#  
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#  
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA
# Questions? Contact Bill Spotz (wfspotz@sandia.gov) 
# 
# ************************************************************************
# @HEADER

# -------------------------------------------------------------------------- #
# This example shows how to derive class MLAPI_BaseOperator to define 
# smoothers in the ML module of Python. Class `MyJacobiSmoother' is a 
# very simple example of the procedure to follow. Basically, one just has
# to modify Apply(), by replacing the Jacobi smoother with the operator
# of choice. 
#
# This multilevel cycle is applied by function MultiLevelCycle(). A more
# elegant example is reported in file exMLAPI.py
#
# \author Marzio Sala, SNL 9214
# 
# \date Last updated on 05-Aug-05
# -------------------------------------------------------------------------- #

from   optparse import *
import sys

parser = OptionParser()
parser.add_option("-b", "--use-boost", action="store_true", dest="boost",
                  default=False,
                  help="test the experimental boost-generated PyTrilinos package")
parser.add_option("-t", "--testharness", action="store_true",
                  dest="testharness", default=False,
                  help="test local build modules; prevent loading system-installed modules")
parser.add_option("-v", "--verbosity", type="int", dest="verbosity", default=2,
                  help="set the verbosity level [default 2]")
options,args = parser.parse_args()
if options.testharness:
    import setpath
    if options.boost: setpath.setpath("src-boost")
    else:             setpath.setpath()
    import Epetra
    import ML
else:
    try:
        import setpath
        if options.boost: setpath.setpath("src-boost")
        else:             setpath.setpath()
        import Epetra
        import ML
    except:
        from PyTrilinos import Epetra
        from PyTrilinos import ML
        print >>sys.stderr, "Using system-installed versions of Epetra, ML"

################################################################################

# ----------------------- #
# User's Defined Smoother #
# ----------------------- #
#
class MyJacobiSmoother(ML.BaseOperator): 
  def Reshape(self, A, omega, sweeps):
    self.A_ = A
    self.DomainSpace_ = A.GetOperatorDomainSpace()
    self.RangeSpace_  = A.GetOperatorRangeSpace()
    self.omega_ = omega
    self.sweeps_ = sweeps
    # Builds here a matrix whose diagonal entries are 1/a_{i,i}
    # This is only one way of doing things, here reported for simplicity.
    self.D_ = ML.PyMatrix(self.RangeSpace_, self.DomainSpace_)
    for i in self.DomainSpace_.GetMyGlobalElements():
      self.D_[i, i] = 1.0 / A[i, i]
    self.D_.FillComplete()

  # A note on Apply(): Recall that if you reassign the x object,
  # then the original x can remain untouched... For example,
  # something like
  #    x = x + y
  # means that a new `x' is created. For this reason, use the Update()
  # methods instead.
  def Apply(*args):
    self = args[0]
    y = args[1]
    x = args[2]
    for j in xrange(self.sweeps_):
      res = y - self.A_ * x
      x.Update(self.omega_, self.D_ * res, 1.0)
    return(0)

  def GetOperatorDomainSpace(self):
    return(self.DomainSpace_)

  def GetOperatorRangeSpace(self):
    return(self.RangeSpace_)

  def __str__(self):
    return "MyJacobiSmoother"

  def __mul__(*args):
    self = args[0]
    rhs = args[1]
    res = ML.MultiVector(rhs.GetVectorSpace())
    self.Apply(rhs, res)
    return(res)

################################################################################

# -------------------------------------- #
# Function to apply the multilevel cycle #
# to vector b_f, and return the result.  #
# -------------------------------------- #
#
def MultiLevelCycle(A, P, R, S, b_f, level, MaxLevels):
  if level == MaxLevels - 1:
    return(S[level] * b_f)
  # apply pre-smoother
  x_f = S[level] * b_f
  # new residual
  r_f = b_f - A[level] * x_f
  # restrict to coarse
  r_c = R[level] * r_f
  # solve coarse problem
  z_c = MultiLevelCycle(A, P, R, S, r_c, level + 1, MaxLevels)
  # prolongate back and add to solution
  x_f = x_f + P[level] * z_c
  # apply post-smoother
  S[level].Apply(b_f, x_f)
  return(x_f)

################################################################################

# ----------- #
# Main driver #
# ----------- #
#
def main():
  # Defines a communicator (serial or parallel, depending on how Trilinos
  # was configured), and creates a matrix corresponding to a 1D Laplacian.
  Comm = Epetra.PyComm()

  n = 1000
  FineSpace = ML.Space(n)
  
  Matrix = ML.PyMatrix(FineSpace, FineSpace)
  for i in xrange(n):
    if i > 0:
      Matrix[i, i - 1] = -1.
    if i < n - 1:
      Matrix[i, i + 1] = -1.
    Matrix[i, i] = 2.0
  Matrix.FillComplete()

  # Declares the lists that will contain the matrices (A), the prolongator
  # operators (P), the restriction operator (R), and the smoother or
  # coarse solver (S). The finest level matrix will be stored in A[0].
  A = [Matrix]  
  P = []
  R = []
  S = []

  NullSpace = ML.MultiVector(Matrix.GetDomainSpace())
  NullSpace.Update(1.0)
  
  List = {
    "aggregation: type": "MIS",
    "PDE equations": 1
  }

  # -------------------------------------------- #
  # Start the construction of the preconditioner #
  # -------------------------------------------- #
  #
  MaxLevels = 3
  for level in xrange(MaxLevels):
    NextNullSpace = ML.MultiVector()
    # Constructs the non-smoothed prolongator...
    Ptent = ML.GetPNonSmoothed(A[level], NullSpace, NextNullSpace, List)
    # ...and smooth it
    I = ML.GetIdentity(A[level].GetRangeSpace(), A[level].GetRangeSpace())
    lambda_max = 1.0 / ML.MaxEigAnorm(A[level])
    P_level = (I - A[level] * lambda_max) * Ptent
    # Stores prolongator, restriction, and RAP product
    P.append((I - A[level] * lambda_max) * Ptent)
    R.append(ML.GetTranspose(P[level]))
    A.append(ML.GetRAP(R[level], A[level], P[level]))
    # Defines the coarse solver or smoother (symmetric Gauss-Seidel)
    if level == MaxLevels - 1:
      S.append(ML.InverseOperator(A[level], "Amesos"))
    else:
      S_level = MyJacobiSmoother()
      S_level.Reshape(A[level], 0.67, 2)
      S.append(S_level)
    NullSpace = NextNullSpace  
  
  # Defines a linear system with a given exact solution
  x = ML.MultiVector(FineSpace)
  x_exact = ML.MultiVector(FineSpace)
  x_exact.Random()
  x_exact_norm = x_exact.Norm2()
  y = A[0] * x_exact
  
  # ---------------------------------------------------- #
  # A very simple Richardson method to solve the problem #
  # using 10 iterations:                                 #
  # ---------------------------------------------------- #
  #
  for i in xrange(10):
    # compute non-preconditioned residual
    res = y - A[0] * x 
    # compute the preconditioned residual
    prec_res = MultiLevelCycle(A, P, R, S, res, 0, MaxLevels)
    # update the solution vector
    x = x + prec_res 
    # compute the energy of the error
    diff = x - x_exact
    norm = diff * (A[0] * diff)
    if Comm.MyPID() == 0:
      print "iter ", i, " ||x - x_exact||_A = ", norm
      print "End Result: TEST PASSED"

################################################################################

if __name__ == "__main__":
  main()
