#! /usr/bin/env python

import sys

try:
  import setpath
  import Epetra, EpetraExt
except ImportError:
    from PyTrilinos import Epetra, EpetraExt
    print >>sys.stderr, "Using system-installed Epetra, EptraExt"

def main():

  failures = 0
  tolerance = 1.0e-12

  # Construct a vector x and populate with random values
  comm = Epetra.PyComm()
  n    = 10 * comm.NumProc()
  map  = Epetra.Map(n, 0, comm)
  x    = Epetra.Vector(map)
  x.Random()

  # ==================================================== #
  # Write map to file "map.mm" in MatrixMarket format,   #
  # read "map.mm" into map2, then check that map2 equals #
  # map.                                                 #
  # ==================================================== #

  if comm.MyPID() == 0: print "I/O for BlockMap ... ",
  EpetraExt.BlockMapToMatrixMarketFile("map.mm", map)
  (ierr, map2) = EpetraExt.MatrixMarketFileToBlockMap("map.mm", comm)
  if map2.SameAs(map):
    if comm.MyPID() == 0: print "ok"
  else:
    if comm.MyPID() == 0: print "FAILED"
    failures += 1

  # ===================================================== #
  # Write vector x to file "x.mm" in MatrixMarket format, #
  # read "x.mm" into y, then check that y equals x        #
  # ===================================================== #

  if comm.MyPID() == 0: print "I/O for MultiVector ... ",
  EpetraExt.MultiVectorToMatrixMarketFile("x.mm", x)
  (ierr, y) = EpetraExt.MatrixMarketFileToMultiVector("x.mm", map)
  y.Update(1.0, x, -1.0)
  (ierr,norm) = y.Norm2()

  if abs(norm) < tolerance:
    if comm.MyPID() == 0: print "ok"
  else:
    if comm.MyPID() == 0: print "FAILED"
    failures += 1

  # ===================================================== #
  # Creates a simple CrsMatrix (diagonal) and             #
  # write matrix A to file "A.mm" in MatrixMarket format, #
  # read "A.mm" into B, then check that B equals A        #
  # ===================================================== #

  if comm.MyPID() == 0: print "I/O for CrsMatrix ... ",
  A       = Epetra.CrsMatrix(Epetra.Copy, map, 0)
  Indices = Epetra.IntSerialDenseVector(1)
  Values  = Epetra.SerialDenseVector(1)
  for lrid in range(A.NumMyRows()):
    grid = A.GRID(lrid)
    Indices[0] = grid
    Values[0]  = grid
    A.InsertGlobalValues(grid, 1, Values, Indices)
  A.FillComplete()
  EpetraExt.RowMatrixToMatrixMarketFile("A.mm", A)
  (ierr, B) = EpetraExt.MatrixMarketFileToCrsMatrix("A.mm", map)
  EpetraExt.Add(A, False, 1.0, B, -1.0)
  norm = B.NormInf()

  if abs(norm) < tolerance:
    if comm.MyPID() == 0: print "ok"
  else:
    if comm.MyPID() == 0: print "FAILED"
    failures += 1

  return failures

################################################################

if __name__ == "__main__":
  failures = main()
  sys.exit(failures)
