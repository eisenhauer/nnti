#! /bin/env python
from PyTrilinos import Epetra, AztecOO, Amesos, IFPACK, ML
from Numeric import *

Comm = Epetra.PyComm()

NumGlobalElements = 10
Map = Epetra.Map(NumGlobalElements, 0, Comm)
MyGlobalElements = Map.MyGlobalElements()

Matrix = Epetra.CrsMatrix(Epetra.Copy, Map, 0)

for i in MyGlobalElements:
  Matrix[i, i] = 1.0

Matrix.FillComplete()

MLList = {
  "max levels"        : 3,
  "output"            : 10,
  "smoother: type"    : "symmetric Gauss-Seidel",
  "aggregation: type" : "Uncoupled"
}

Prec = ML.MultiLevelPreconditioner(Matrix, False)
Prec.SetParameterList(MLList)
Prec.ComputePreconditioner()
