#! /usr/bin/env python

# @header
# ************************************************************************
#
#           PyTrilinos.EpetraExt: Python interface to EpetraExt
#                   Copyright (2005) Sandia Corporation
#
# Under terms of contract DE-AC04-94AL85000, there is a non-exclusive
# license for use of this work by or on behalf of the U.S. Government.
#
# This library is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 2.1 of the
# license, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful, but
# without any warranty; without even the implied warranty of
# merchantability or fitness for a particular purpose.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA
# Questions? contact Michael A. Heroux (maherou@sandia.gov)
#
# ************************************************************************
# @header

import sys

try:
  import setpath
  import Epetra, EpetraExt
except ImportError:
    from PyTrilinos import Epetra, EpetraExt
    print >>sys.stderr, "Using system-installed Epetra, EptraExt"

# Build a global communicator
comm    = Epetra.PyComm()
numProc = comm.NumProc()
iAmRoot = comm.MyPID() == 0

def main():

  failures  = 0
  tolerance = 1.0e-12

  # Construct a vector x and populate with random values
  n       = 10 * numProc
  map     = Epetra.Map(n, 0, comm)
  x       = Epetra.Vector(map)
  x.Random()

  # ==================================================== #
  # Write map to file "map.mm" in MatrixMarket format,   #
  # read "map.mm" into map2, then check that map2 equals #
  # map.                                                 #
  # ==================================================== #

  if iAmRoot: print "I/O for Map ... ",
  EpetraExt.BlockMapToMatrixMarketFile("map.mm", map)
  (ierr, map2) = EpetraExt.MatrixMarketFileToMap("map.mm", comm)
  print "ierr =", ierr
  print "map2 =", map2
  if map2.SameAs(map):
    if iAmRoot: print "ok"
  else:
    if iAmRoot: print "FAILED"
    failures += 1

  # ===================================================== #
  # Write vector x to file "x.mm" in MatrixMarket format, #
  # read "x.mm" into y, then check that y equals x        #
  # ===================================================== #

  if iAmRoot: print "I/O for MultiVector ... ",
  EpetraExt.MultiVectorToMatrixMarketFile("x.mm", x)
  (ierr, y) = EpetraExt.MatrixMarketFileToMultiVector("x.mm", map2)
  y.Update(1.0, x, -1.0)
  norm = y.Norm2()

  if abs(norm) < tolerance:
    if iAmRoot: print "ok"
  else:
    if iAmRoot: print "FAILED"
    failures += 1

  # ===================================================== #
  # Creates a simple CrsMatrix (diagonal) and             #
  # write matrix A to file "A.mm" in MatrixMarket format, #
  # read "A.mm" into B, then check that B equals A        #
  # ===================================================== #

  if iAmRoot: print "I/O for CrsMatrix ... ",
  A       = Epetra.CrsMatrix(Epetra.Copy, map2, 0)
  Indices = Epetra.IntSerialDenseVector(1)
  Values  = Epetra.SerialDenseVector(1)
  for lrid in range(A.NumMyRows()):
    grid = A.GRID(lrid)
    Indices[0] = grid
    Values[0]  = grid
    A.InsertGlobalValues(grid, 1, Values, Indices)
  A.FillComplete()
  EpetraExt.RowMatrixToMatrixMarketFile("A.mm", A)
  (ierr, B) = EpetraExt.MatrixMarketFileToCrsMatrix("A.mm", map2)
  EpetraExt.Add(A, False, 1.0, B, -1.0)
  norm = B.NormInf()

  if abs(norm) < tolerance:
    if iAmRoot: print "ok"
  else:
    if iAmRoot: print "FAILED"
    failures += 1

  return failures

################################################################

if __name__ == "__main__":
  if numProc == 1:
    failures = main() # not all I/O works in parallel
  else
    failure = 0
  failures = comm.SumAll(failures)[0]
  if failures == 0 and iAmRoot: print "End Result: TEST PASSED"
  sys.exit(failures)
