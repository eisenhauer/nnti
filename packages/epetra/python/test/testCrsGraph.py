#! /usr/bin/env python

# @HEADER
# ************************************************************************
#
#              PyTrilinos.Epetra: Python Interface to Epetra
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
# Questions? Contact Michael A. Heroux (maherou@sandia.gov)
#
# ************************************************************************
# @HEADER

# Imports.  Users importing an installed version of PyTrilinos should use the
# "from PyTrilinos import ..." syntax.  Here, the setpath module adds the build
# directory, including "PyTrilinos", to the front of the search path.  We thus
# use "import ..." for Trilinos modules.  This prevents us from accidentally
# picking up a system-installed version and ensures that we are testing the
# build module.
import sys

try:
    import setpath
    import Epetra
except ImportError:
    from PyTrilinos import Epetra
    print >>sys.stderr, "Using system-installed Epetra"

import unittest
from   Numeric    import *

##########################################################################

class EpetraCrsGraphTestCase(unittest.TestCase):
    "TestCase class for Epetra CrsGraphs"

    def setUp(self):
        self.comm      = Epetra.PyComm()
        self.numProc   = self.comm.NumProc()
        self.mySize    = 11
        self.size      = self.mySize * self.numProc
        self.indexBase = 0
        self.rowMap    = Epetra.Map(self.size, self.indexBase, self.comm)
        self.colMap    = Epetra.Map(self.rowMap)
        self.nipr      = ones(self.mySize)
        self.nipr[1:-1] += 1
        self.nipr[2:-2] += 1

    def tearDown(self):
        self.comm.Barrier()

    def fillGraph(self,graph):
        n = self.size
        for lrid in range(graph.NumMyRows()):
            grid = graph.GRID(lrid)
            if   grid == 0  : indices = [0,1]
            elif grid == n-1: indices = [n-2,n-1]
            else            : indices = [grid-1,grid,grid+1]
            graph.InsertGlobalIndices(grid,indices)
        graph.FillComplete()
        

    def testConstructor01(self):
        "Test Epetra.CrsGraph constructor, no colMap w/fixed # of indices/row"
        crsg = Epetra.CrsGraph(Epetra.Copy, self.rowMap, 3)
        self.assertEqual(crsg.NumMyRows(), self.mySize   )
        self.assertEqual(crsg.IndexBase(), self.indexBase)

    def testConstructor02(self):
        "Test Epetra.CrsGraph constructor, no colMap w/fixed # of indices/row, static profile"
        crsg = Epetra.CrsGraph(Epetra.Copy, self.rowMap, 3, True)
        self.assertEqual(crsg.NumMyRows(), self.mySize   )
        self.assertEqual(crsg.IndexBase(), self.indexBase)

    def testConstructor03(self):
        "Test Epetra.CrsGraph constructor, no colMap w/specified # of indices/row"
        crsg = Epetra.CrsGraph(Epetra.Copy, self.rowMap, self.nipr)
        self.assertEqual(crsg.NumMyRows(), self.mySize   )
        self.assertEqual(crsg.IndexBase(), self.indexBase)

    def testConstructor04(self):
        "Test Epetra.CrsGraph constructor, no colMap w/specified # of indices/row, static profile"
        crsg = Epetra.CrsGraph(Epetra.Copy, self.rowMap, self.nipr, True)
        self.assertEqual(crsg.NumMyRows(), self.mySize   )
        self.assertEqual(crsg.IndexBase(), self.indexBase)

    def testConstructor05(self):
        "Test Epetra.CrsGraph constructor, w/colMap w/fixed # of indices/row"
        try:
            crsg = Epetra.CrsGraph(Epetra.Copy, self.rowMap, self.colMap, 3)
            self.assertEqual(crsg.NumMyRows(), self.mySize   )
            self.assertEqual(crsg.IndexBase(), self.indexBase)
        except TypeError:
            # A TypeError is raised for the wrapper generated by older versions
            # of SWIG (1.3.28 and earlier).  This is expected, so just ignore it
            pass

    def testConstructor06(self):
        "Test Epetra.CrsGraph constructor, w/colMap w/fixed # of indices/row, static profile"
        crsg = Epetra.CrsGraph(Epetra.Copy, self.rowMap, self.colMap, 3, True)
        self.assertEqual(crsg.NumMyRows(), self.mySize   )
        self.assertEqual(crsg.IndexBase(), self.indexBase)

    def testConstructor07(self):
        "Test Epetra.CrsGraph constructor, w/colMap w/specified # of indices/row"
        crsg = Epetra.CrsGraph(Epetra.Copy, self.rowMap, self.colMap, self.nipr)
        self.assertEqual(crsg.NumMyRows(), self.mySize   )
        self.assertEqual(crsg.IndexBase(), self.indexBase)

    def testConstructor08(self):
        "Test Epetra.CrsGraph constructor, w/colMap w/specified # of indices/row, static profile"
        crsg = Epetra.CrsGraph(Epetra.Copy, self.rowMap, self.colMap, self.nipr, True)
        self.assertEqual(crsg.NumMyRows(), self.mySize   )
        self.assertEqual(crsg.IndexBase(), self.indexBase)

    def testConstructor09(self):
        "Test Epetra.CrsGraph copy constructor"
        crsg1 = Epetra.CrsGraph(Epetra.Copy, self.rowMap, self.nipr)
        crsg2 = Epetra.CrsGraph(crsg1)
        self.assertEqual(crsg2.NumMyRows(), self.mySize   )
        self.assertEqual(crsg2.IndexBase(), self.indexBase)

    def testConstructor10(self):
        "Test Epetra.CrsGraph constructor with too-short array"
        self.assertRaises(ValueError, Epetra.CrsGraph, Epetra.Copy, self.rowMap,
                          self.nipr[1:-1])

    def testConstructor11(self):
        "Test Epetra.CrsGraph constructor with too-long array"
        nipr = list(self.nipr)
        nipr.append(1)
        self.assertRaises(ValueError, Epetra.CrsGraph, Epetra.Copy, self.rowMap,
                          nipr)

    def testInsertGlobalIndices(self):
        "Test Epetra.CrsGraph InsertGlobalIndices method"
        crsg = Epetra.CrsGraph(Epetra.Copy, self.rowMap, 3)
        self.fillGraph(crsg)  # This calls crsg.InsertGlobalIndices()
        n = self.size
        for lrid in range(crsg.NumMyRows()):
            grid = crsg.GRID(lrid)
            if grid in (0,n-1): numIndices = 2
            else:               numIndices = 3
            self.assertEqual(crsg.NumMyIndices(lrid), numIndices)

    def testInsertGlobalIndicesBad(self):
        "Test Epetra.CrsGraph InsertGlobalIndices method for bad indices"
        crsg = Epetra.CrsGraph(Epetra.Copy, self.rowMap, 3)
        grid = crsg.GRID(0)
        self.assertRaises(ValueError, crsg.InsertGlobalIndices, grid, [0,"e","pi"])

    #def testRemoveGlobalIndices(self):
    #    "Test Epetra.CrsGraph RemoveGlobalIndices method"
    #    crsg = Epetra.CrsGraph(Epetra.Copy, self.rowMap, 3)
    #    self.fillGraph(crsg)
    #    grid = crsg.GRID(0)
    #    indices = crsg.ExtractGlobalRowCopy(grid)
    #    print indices
    #    crsg.RemoveGlobalIndices(grid,indices[1:])
    #    self.assertEqual(crsg.NumMyIndices(0), 1)

    #def testInsertMyIndices(self):
    #    "Test Epetra.CrsGraph InsertMyIndices method"
    #    crsg = Epetra.CrsGraph(Epetra.Copy, self.map, 3)
    #    # The following FillComplete() call creates a column map for crsg, which
    #    # is required when using local indices
    #    crsg.FillComplete()
    #    self.assert_(crsg.InsertMyIndices(0,2,array([0,1])) >= 0)
    #    for i in range(1,self.size-1):
    #        self.assert_(crsg.InsertMyIndices(i,3,array([i-1,i,i+1])) >= 0)
    #    self.assert_(crsg.InsertMyIndices(self.size-1,2,
    #                                      array([self.size-2,self.size-1])) >= 0)
    #    crsg.FillComplete()
    #    print crsg

    def testExtractGlobalRowCopy(self):
        "Test Epetra.CrsGraph ExtractGlobalRowCopy method"
        crsg = Epetra.CrsGraph(Epetra.Copy, self.rowMap, 3)
        self.fillGraph(crsg)
        n = crsg.GRID(self.mySize-1)
        indices = crsg.ExtractGlobalRowCopy(n)
        self.assertEqual(len(indices), 2  )
        self.assertEqual(indices[0]  , n-1)
        self.assertEqual(indices[1]  , n  )

    def testExtractGlobalRowCopyBad(self):
        "Test Epetra.CrsGraph ExtractGlobalRowCopy method, bad index"
        crsg = Epetra.CrsGraph(Epetra.Copy, self.rowMap, 3)
        self.fillGraph(crsg)
        self.assertRaises(ValueError, crsg.ExtractGlobalRowCopy, self.size)

    def testExtractMyRowCopy(self):
        "Test Epetra.CrsGraph ExtractMyRowCopy method"
        crsg = Epetra.CrsGraph(Epetra.Copy, self.rowMap, 3)
        self.fillGraph(crsg)
        n = self.mySize-1
        indices = crsg.ExtractMyRowCopy(n)
        self.assertEqual(len(indices), 2  )
        self.assertEqual(indices[0]  , n-1)
        self.assertEqual(indices[1]  , n  )

    def testExtractMyRowCopyBad(self):
        "Test Epetra.CrsGraph ExtractMyRowCopy method, bad index"
        crsg = Epetra.CrsGraph(Epetra.Copy, self.rowMap, 3)
        self.fillGraph(crsg)
        self.assertRaises(ValueError, crsg.ExtractMyRowCopy, self.mySize)

##########################################################################

if __name__ == "__main__":

    # Create the test suite object
    suite = unittest.TestSuite()

    # Add the test cases to the test suite
    suite.addTest(unittest.makeSuite(EpetraCrsGraphTestCase))

    # Create a communicator
    comm    = Epetra.PyComm()
    iAmRoot = comm.MyPID() == 0

    # Run the test suite
    if iAmRoot: print >>sys.stderr, \
       "\n***********************\nTesting Epetra.CrsGraph\n***********************\n"
    verbosity = 2 * int(iAmRoot)
    result = unittest.TextTestRunner(verbosity=verbosity).run(suite)

    # Exit with a code that indicates the total number of errors and failures
    errsPlusFails = comm.SumAll(len(result.errors) + len(result.failures))[0]
    if errsPlusFails == 0 and iAmRoot: print "End Result: TEST PASSED"
    sys.exit(errsPlusFails)
