#! /usr/bin/env python

# "from PyTrilinos import ..." syntax.  Here, the setpath module adds the build
# directory, including "PyTrilinos", to the front of the search path.  We thus
# use "import ..." for Trilinos modules.  This prevents us from accidentally
# picking up a system-installed version and ensures that we are testing the
# build module.
try:
  import setpath
  import Epetra, Galeri
except ImportError:
  from PyTrilinos import Epetra, Galeri
  print "Using system-installed Epetra, Galeri"

# Creates a communicator, which is an Epetra_MpiComm if Trilinos was
# configured with MPI support, serial otherwise.
Comm = Epetra.PyComm()

# Reads the matrix from file ``gre__115.rua'', downloaded from
# the MatrixMarket web site. Use the try/except block to
# catch the integer exception that is thrown if the matrix file
# cannot be opened
try:
  Map, Matrix, X, B, Xexact = Galeri.ReadHB("gre__q115.rua", Comm);
except:
  print "Problems reading matrix file, perhaps you are in"
  print "the wrong directory"

# at this point you can use the objects in any PyTrilinos module,
# for example AztecOO, Amesos, IFPACK, ML, and so on. 
