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
from   numpy    import *
from   optparse import *
import os
import sys
import unittest

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
else:
    try:
        import setpath
        if options.boost: setpath.setpath("src-boost")
        else:             setpath.setpath()
        import Epetra
    except ImportError:
        from PyTrilinos import Epetra
        print >>sys.stderr, "Using system-installed Epetra"

##########################################################################

class EpetraPyCommTestCase(unittest.TestCase):
    "TestCase class for PyComm communicator objects"

    def setUp(self):
        self.comm  = Epetra.PyComm()
        if self.comm.Label() == "Epetra::MpiComm":
            self.commType = "MPI"
            self.output   = "  Processor %d of %d total processors" % \
                            (self.comm.MyPID(), self.comm.NumProc()) 
        else:
            self.commType = "Serial"
            self.output   = "::Processor 0 of 1 total processors."
        self.comm.Barrier()

    def tearDown(self):
        self.comm.Barrier()

    def testMyPID(self):
        "Test Epetra.PyComm MyPID method"
        pid = self.comm.MyPID()
        self.assert_(pid < self.comm.NumProc())
        if self.commType == "Serial":
            self.assertEqual(pid, 0)
        else:
            self.assert_(pid >= 0)

    def testNumProc(self):
        "Test Epetra.PyComm NumProc method"
        n = self.comm.NumProc()
        if self.commType == "Serial":
            self.assertEqual(n, 1)
        else:
            self.assert_(n > 0)

    def testStr(self):
        "Test Epetra.PyComm __str__ method"
        self.assertEqual(str(self.comm), self.output)

    def testPrint(self):
        "Test Epetra.PyComm Print method"
        filename = "testComm%d.dat" % self.comm.MyPID()
        f = open(filename, "w")
        self.comm.Print(f)
        f.close()
        f = open(filename, "r")
        self.assertEqual(f.read(), self.output)
        f.close()

    def testBroadcastBadArg(self):
        "Test Epetra.PyComm Broadcast method with bad argument"
        self.assertRaises(TypeError, self.comm.Broadcast, None, 0)

    def testBroadcastBadType(self):
        "Test Epetra.PyComm Broadcast method with bad type array"
        myArray = ones((3,3),'f')  # Float type not supported by Epetra
        self.assertRaises(TypeError, self.comm.Broadcast, myArray, 0)

    def testBroadcastNoncontiguous(self):
        "Test Epetra.PyComm Broadcast method with noncontiguous array"
        baseArray = zeros((5,5),'d')
        myArray   = baseArray[1:3,1:3]  # Noncontiguous array
        self.assertRaises(TypeError, self.comm.Broadcast, myArray, 0)

    def testBroadcastInt(self):
        "Test Epetra.PyComm Broadcast method for ints"
        rootVals = array([5,0,4,1,3,2],'i')
        if self.comm.MyPID() == 0:
            myVals = array(rootVals)
        else:
            myVals = array([-1,-1,-1,-1,-1,-1],'i')
        self.comm.Broadcast(myVals, 0)
        for i in range(len(rootVals)):
            self.assertEquals(myVals[i], rootVals[i])

    def testBroadcastInt2D(self):
        "Test Epetra.PyComm Broadcast method for 2D int array"
        rootVals = array([[5,0],[4,1]],'i')
        if self.comm.MyPID() == 0:
            myVals = array(rootVals)
        else:
            myVals = array([[-1,-1],[-1,-1]],'i')
        self.comm.Broadcast(myVals, 0)
        for i in range(2):
            for j in range(2):
                self.assertEquals(myVals[i,j], rootVals[i,j])

    def testBroadcastLong(self):
        "Test Epetra.PyComm Broadcast method for longs"
        rootVals = array([5,0,4,1,3,2])
        if self.comm.MyPID() == 0:
            myVals = array(rootVals)
        else:
            myVals = array([-1,-1,-1,-1,-1,-1])
        self.comm.Broadcast(myVals, 0)
        for i in range(len(rootVals)):
            self.assertEquals(myVals[i], rootVals[i])

    def testBroadcastLong2D(self):
        "Test Epetra.PyComm Broadcast method for 2D long array"
        rootVals = array([[5,0],[4,1]])
        if self.comm.MyPID() == 0:
            myVals = array(rootVals)
        else:
            myVals = array([[-1,-1],[-1,-1]])
        self.comm.Broadcast(myVals, 0)
        for i in range(2):
            for j in range(2):
                self.assertEquals(myVals[i,j], rootVals[i,j])

    def testBroadcastDouble(self):
        "Test Epetra.PyComm Broadcast method for doubles"
        rootVals = array([5,0,4,1,3,2],dtype='d')
        if self.comm.MyPID() == 0:
            myVals = array(rootVals)
        else:
            myVals = array([-1,-1,-1,-1,-1,-1],dtype='d')
        self.comm.Broadcast(myVals, 0)
        for i in range(len(rootVals)):
            self.assertEquals(myVals[i], rootVals[i])

    def testBroadcastDouble2D(self):
        "Test Epetra.PyComm Broadcast method for 2D double array"
        rootVals = array([[5,0],[4,1]],'d')
        if self.comm.MyPID() == 0:
            myVals = array(rootVals)
        else:
            myVals = array([[-1,-1],[-1,-1]],'d')
        self.comm.Broadcast(myVals, 0)
        for i in range(2):
            for j in range(2):
                self.assertEquals(myVals[i,j], rootVals[i,j])

    def testGatherAllBadArg(self):
        "Test Epetra.PyComm GatherAll method with bad arguments"
        self.assertRaises(TypeError, self.comm.GatherAll, None)

    def testGatherAllBadType(self):
        "Test Epetra.PyComm GatherAll method with bad type array"
        myArray  = ones((3,3),'f')  # Float type not supported by Epetra
        self.assertRaises(TypeError, self.comm.GatherAll, myArray)

    def testGatherAllInt(self):
        "Test Epetra.PyComm GatherAll method for ints"
        myPID      = self.comm.MyPID()
        length     = 4*self.comm.NumProc()
        globalVals = arange(length, dtype='i')
        myVals     = array(globalVals[4*myPID:4*(myPID+1)])
        allVals    = self.comm.GatherAll(myVals)
        allVals.shape = (length,)
        for i in range(len(globalVals)):
            self.assertEquals(allVals[i], globalVals[i])

    def testGatherAllInt2D(self):
        "Test Epetra.PyComm GatherAll method for 2D int array"
        myPID      = self.comm.MyPID()
        numProc    = self.comm.NumProc()
        globalVals = zeros((numProc,2,2),'i')
        for i in range(numProc):
            globalVals[i,:,:] = [[1,2],[3,4]]
        myVals     = globalVals[myPID,:,:]
        allVals    = self.comm.GatherAll(myVals)
        for p in range(numProc):
            for i in range(2):
                for j in range(2):
                    self.assertEquals(allVals[p,i,j], globalVals[p,i,j])

    def testGatherAllIntNoncontiguous(self):
        "Test Epetra.PyComm GatherAll method with noncontiguous int array"
        baseArray = arange(20,dtype='i')
        baseArray.shape = (4,5)
        myArray   = baseArray[1:3,1:4] # Noncontiguous
        allArray  = self.comm.GatherAll(myArray)
        numProc   = self.comm.NumProc()
        self.assertEquals(allArray.shape, (numProc,2,3))
        for p in range(numProc):
            self.failUnless((allArray[p,:,:] == myArray).all())

    def testGatherAllLong(self):
        "Test Epetra.PyComm GatherAll method for longs"
        myPID      = self.comm.MyPID()
        length     = 4*self.comm.NumProc()
        globalVals = arange(length, dtype='i')
        myVals     = array(globalVals[4*myPID:4*(myPID+1)])
        allVals    = self.comm.GatherAll(myVals)
        allVals.shape = (length,)
        for i in range(len(globalVals)):
            self.assertEquals(allVals[i], globalVals[i])

    def testGatherAllLong2D(self):
        "Test Epetra.PyComm GatherAll method for 2D long array"
        myPID      = self.comm.MyPID()
        numProc    = self.comm.NumProc()
        globalVals = zeros((numProc,2,2),'i')
        for i in range(numProc):
            globalVals[i,:,:] = [[1,2],[3,4]]
        myVals     = globalVals[myPID,:,:]
        allVals    = self.comm.GatherAll(myVals)
        for p in range(numProc):
            for i in range(2):
                for j in range(2):
                    self.assertEquals(allVals[p,i,j], globalVals[p,i,j])

    def testGatherAllLongNoncontiguous(self):
        "Test Epetra.PyComm GatherAll method with noncontiguous long array"
        baseArray = arange(20)
        baseArray.shape = (4,5)
        myArray   = baseArray[1:3,1:4] # Noncontiguous
        allArray  = self.comm.GatherAll(myArray)
        numProc   = self.comm.NumProc()
        self.assertEquals(allArray.shape, (numProc,2,3))
        for p in range(numProc):
            self.failUnless((allArray[p,:,:] == myArray).all())

    def testGatherAllDouble(self):
        "Test Epetra.PyComm GatherAll method for doubles"
        myPID      = self.comm.MyPID()
        length     = 4*self.comm.NumProc()
        globalVals = arange(length, dtype='d')
        myVals     = array(globalVals[4*myPID:4*(myPID+1)])
        allVals    = self.comm.GatherAll(myVals)
        allVals.shape = (length,)
        for i in range(len(globalVals)):
            self.assertEquals(allVals[i], globalVals[i])

    def testGatherAllDouble2D(self):
        "Test Epetra.PyComm GatherAll method for 2D double array"
        myPID      = self.comm.MyPID()
        numProc    = self.comm.NumProc()
        globalVals = zeros((numProc,2,2),'d')
        for i in range(numProc):
            globalVals[i,:,:] = [[1,2],[3,4]]
        myVals     = globalVals[myPID,:,:]
        allVals    = self.comm.GatherAll(myVals)
        for p in range(len(globalVals)):
            for i in range(2):
                for j in range(2):
                    self.assertEquals(allVals[p,i,j], globalVals[p,i,j])

    def testGatherAllDoubleNoncontiguous(self):
        "Test Epetra.PyComm GatherAll method with noncontiguous double array"
        baseArray = arange(20,dtype='d')
        baseArray.shape = (5,4)
        myArray   = baseArray[1:4,1:3] # Noncontiguous
        allArray  = self.comm.GatherAll(myArray)
        numProc   = self.comm.NumProc()
        self.assertEquals(allArray.shape, (numProc,3,2))
        for i in range(numProc):
            self.failUnless((allArray[i,:,:] == myArray).all())

    def testSumAllBadArg(self):
        "Test Epetra.PyComm SumAll method with bad arguments"
        self.assertRaises(TypeError, self.comm.SumAll, None)

    def testSumAllBadType(self):
        "Test Epetra.PyComm SumAll method with bad type array"
        partialSums = ones((3,3),'f')  # Float type not supported by Epetra
        self.assertRaises(TypeError, self.comm.SumAll, partialSums)

    def testSumAllInt(self):
        "Test Epetra.PyComm SumAll method for ints"
        numProc     = self.comm.NumProc()
        partialSums = arange(3,dtype='i')
        globalSums  = self.comm.SumAll(partialSums)
        for i in range(len(partialSums)):
            self.assertEquals(globalSums[i], numProc*partialSums[i])

    def testSumAllInt2D(self):
        "Test Epetra.PyComm SumAll method for 2D int array"
        numProc     = self.comm.NumProc()
        partialSums = arange(12,dtype='i')
        partialSums.shape = (3,4)
        globalSums  = self.comm.SumAll(partialSums)
        for i in range(3):
            for j in range(4):
                self.assertEquals(globalSums[i,j], numProc*partialSums[i,j])

    def testSumAllIntNoncontiguous(self):
        "Test Epetra.PyComm SumAll method with noncontiguous int array"
        numProc     = self.comm.NumProc()
        baseArray   = arange(15,dtype='i')
        baseArray.shape = (3,5)
        partialSums = baseArray[1:2,1:4]  # Noncontiguous
        globalSums  = self.comm.SumAll(partialSums)
        for i in range(1):
            for j in range(3):
                self.assertEquals(globalSums[i,j], numProc*partialSums[i,j])

    def testSumAllLong(self):
        "Test Epetra.PyComm SumAll method for longs"
        numProc     = self.comm.NumProc()
        partialSums = arange(3)
        globalSums  = self.comm.SumAll(partialSums)
        for i in range(len(partialSums)):
            self.assertEquals(globalSums[i], numProc*partialSums[i])

    def testSumAllLong2D(self):
        "Test Epetra.PyComm SumAll method for 2D long array"
        numProc     = self.comm.NumProc()
        partialSums = arange(12)
        partialSums.shape = (3,4)
        globalSums  = self.comm.SumAll(partialSums)
        for i in range(3):
            for j in range(4):
                self.assertEquals(globalSums[i,j], numProc*partialSums[i,j])

    def testSumAllLongNoncontiguous(self):
        "Test Epetra.PyComm SumAll method with noncontiguous long array"
        numProc     = self.comm.NumProc()
        baseArray   = arange(15)
        baseArray.shape = (3,5)
        partialSums = baseArray[1:2,1:4]  # Noncontiguous
        globalSums  = self.comm.SumAll(partialSums)
        for i in range(1):
            for j in range(3):
                self.assertEquals(globalSums[i,j], numProc*partialSums[i,j])

    def testSumAllDouble(self):
        "Test Epetra.PyComm SumAll method for doubles"
        numProc     = self.comm.NumProc()
        partialSums = arange(4,dtype='d')
        globalSums  = self.comm.SumAll(partialSums)
        for i in range(len(partialSums)):
            self.assertEquals(globalSums[i], numProc*partialSums[i])

    def testSumAllDouble2D(self):
        "Test Epetra.PyComm SumAll method for 2D double array"
        numProc     = self.comm.NumProc()
        partialSums = arange(15,dtype='d')
        partialSums.shape = (5,3)
        globalSums  = self.comm.SumAll(partialSums)
        for i in range(5):
            for j in range(3):
                self.assertEquals(globalSums[i,j], numProc*partialSums[i,j])

    def testSumAllDoubleNoncontiguous(self):
        "Test Epetra.PyComm SumAll method with noncontiguous double array"
        numProc     = self.comm.NumProc()
        baseArray   = arange(20,dtype='d')
        baseArray.shape = (5,4)
        partialSums = baseArray[1:4,1:3]  # Noncontiguous
        globalSums  = self.comm.SumAll(partialSums)
        for i in range(3):
            for j in range(2):
                self.assertEquals(globalSums[i,j], numProc*partialSums[i,j])

    def testMaxAllBadArg(self):
        "Test Epetra.PyComm MaxAll method with bad arguments"
        self.assertRaises(TypeError, self.comm.MaxAll, None)

    def testMaxAllBadType(self):
        "Test Epetra.PyComm MaxAll method with bad type array"
        partialMaxs = ones((3,3),'f')  # Float type not supported by Epetra
        self.assertRaises(TypeError, self.comm.MaxAll, partialMaxs)

    def testMaxAllInt(self):
        "Test Epetra.PyComm MaxAll method for ints"
        numProc     = self.comm.NumProc()
        baseArray   = arange(5,dtype='i')
        partialMaxs = array(baseArray) * (self.comm.MyPID()+1)
        globalMaxs  = self.comm.MaxAll(partialMaxs)
        for i in range(len(baseArray)):
            self.assertEquals(globalMaxs[i], numProc*baseArray[i])

    def testMaxAllInt2D(self):
        "Test Epetra.PyComm MaxAll method for 2D int array"
        numProc     = self.comm.NumProc()
        baseArray   = arange(9,dtype='i')
        baseArray.shape = (3,3)
        partialMaxs = array(baseArray) * (self.comm.MyPID()+1)
        globalMaxs  = self.comm.MaxAll(partialMaxs)
        for i in range(3):
            for j in range(3):
                self.assertEquals(globalMaxs[i,j], numProc*baseArray[i,j])

    def testMaxAllIntNoncontiguous(self):
        "Test Epetra.PyComm MaxAll method with noncontiguous int array"
        numProc     = self.comm.NumProc()
        baseArray   = arange(25,dtype='i')
        baseArray.shape = (5,5)
        baseMaxs    = baseArray[1:4,1:4]  # Noncontiguous
        partialMaxs = baseMaxs * (self.comm.MyPID()+1)
        globalMaxs  = self.comm.MaxAll(partialMaxs)
        for i in range(3):
            for j in range(3):
                self.assertEquals(globalMaxs[i,j], numProc*baseMaxs[i,j])

    def testMaxAllLong(self):
        "Test Epetra.PyComm MaxAll method for longs"
        numProc     = self.comm.NumProc()
        baseArray   = arange(5)
        partialMaxs = array(baseArray) * (self.comm.MyPID()+1)
        globalMaxs  = self.comm.MaxAll(partialMaxs)
        for i in range(len(baseArray)):
            self.assertEquals(globalMaxs[i], numProc*baseArray[i])

    def testMaxAllLong2D(self):
        "Test Epetra.PyComm MaxAll method for 2D long array"
        numProc     = self.comm.NumProc()
        baseArray   = arange(9)
        baseArray.shape = (3,3)
        partialMaxs = array(baseArray) * (self.comm.MyPID()+1)
        globalMaxs  = self.comm.MaxAll(partialMaxs)
        for i in range(3):
            for j in range(3):
                self.assertEquals(globalMaxs[i,j], numProc*baseArray[i,j])

    def testMaxAllLongNoncontiguous(self):
        "Test Epetra.PyComm MaxAll method with noncontiguous long array"
        numProc     = self.comm.NumProc()
        baseArray   = arange(25)
        baseArray.shape = (5,5)
        baseMaxs    = baseArray[1:4,1:4]  # Noncontiguous
        partialMaxs = baseMaxs * (self.comm.MyPID()+1)
        globalMaxs  = self.comm.MaxAll(partialMaxs)
        for i in range(3):
            for j in range(3):
                self.assertEquals(globalMaxs[i,j], numProc*baseMaxs[i,j])

    def testMaxAllDouble(self):
        "Test Epetra.PyComm MaxAll method for doubles"
        numProc     = self.comm.NumProc()
        baseArray   = arange(6,dtype='d')
        partialMaxs = array(baseArray) * (self.comm.MyPID()+1)
        globalMaxs  = self.comm.MaxAll(partialMaxs)
        for i in range(len(baseArray)):
            self.assertEquals(globalMaxs[i], numProc*baseArray[i])

    def testMaxAllDouble2D(self):
        "Test Epetra.PyComm MaxAll method for 2D double array"
        numProc     = self.comm.NumProc()
        baseArray   = arange(16,dtype='d')
        baseArray.shape = (4,4)
        partialMaxs = array(baseArray) * (self.comm.MyPID()+1)
        globalMaxs  = self.comm.MaxAll(partialMaxs)
        for i in range(4):
            for j in range(4):
                self.assertEquals(globalMaxs[i,j], numProc*baseArray[i,j])

    def testMaxAllDoubleNoncontiguous(self):
        "Test Epetra.PyComm MaxAll method with noncontiguous double array"
        numProc     = self.comm.NumProc()
        baseArray   = arange(20,dtype='d')
        baseArray.shape = (4,5)
        baseMaxs    = baseArray[1:3,1:4] # Noncontiguous
        partialMaxs = baseMaxs * (self.comm.MyPID()+1)
        globalMaxs  = self.comm.MaxAll(partialMaxs)
        for i in range(2):
            for j in range(3):
                self.assertEquals(globalMaxs[i,j], numProc*baseMaxs[i,j])

    def testMinAllBadArg(self):
        "Test Epetra.PyComm MinAll method with bad arguments"
        self.assertRaises(TypeError, self.comm.MinAll, None)

    def testMinAllBadType(self):
        "Test Epetra.PyComm MinAll method with bad type array"
        partialMins = ones((3,3),'f')  # Float type not supported by Epetra
        self.assertRaises(TypeError, self.comm.MinAll, partialMins)

    def testMinAllInt(self):
        "Test Epetra.PyComm MinAll method for ints"
        baseArray   = arange(5,dtype='i')
        partialMins = array(baseArray) * (self.comm.MyPID()+1)
        globalMins  = self.comm.MinAll(partialMins)
        for i in range(len(baseArray)):
            self.assertEquals(globalMins[i], baseArray[i])

    def testMinAllInt2D(self):
        "Test Epetra.PyComm MinAll method for 2D int array"
        baseArray    = arange(18,dtype="i")
        baseArray.shape = (3,6)
        partialMins = baseArray * (self.comm.MyPID()+1)
        globalMins  = self.comm.MinAll(partialMins)
        for i in range(3):
            for j in range(6):
                self.assertEquals(globalMins[i,j], baseArray[i,j])

    def testMinAllIntNoncontiguous(self):
        "Test Epetra.PyComm MinAll method with noncontiguous int array"
        baseArray   = arange(24,dtype='i')
        baseArray.shape = (4,6)
        baseArray    = baseArray[1:3,1:5]  # Noncontiguous
        partialMins = baseArray * (self.comm.MyPID()+1)
        globalMins  = self.comm.MinAll(partialMins)
        for i in range(2):
            for j in range(4):
                self.assertEquals(globalMins[i,j], baseArray[i,j])

    def testMinAllLong(self):
        "Test Epetra.PyComm MinAll method for longs"
        baseArray   = arange(5)
        partialMins = array(baseArray) * (self.comm.MyPID()+1)
        globalMins  = self.comm.MinAll(partialMins)
        for i in range(len(baseArray)):
            self.assertEquals(globalMins[i], baseArray[i])

    def testMinAllLong2D(self):
        "Test Epetra.PyComm MinAll method for 2D long array"
        baseArray    = arange(18)
        baseArray.shape = (3,6)
        partialMins = baseArray * (self.comm.MyPID()+1)
        globalMins  = self.comm.MinAll(partialMins)
        for i in range(3):
            for j in range(6):
                self.assertEquals(globalMins[i,j], baseArray[i,j])

    def testMinAllLongNoncontiguous(self):
        "Test Epetra.PyComm MinAll method with noncontiguous long array"
        baseArray   = arange(24)
        baseArray.shape = (4,6)
        baseArray    = baseArray[1:3,1:5]  # Noncontiguous
        partialMins = baseArray * (self.comm.MyPID()+1)
        globalMins  = self.comm.MinAll(partialMins)
        for i in range(2):
            for j in range(4):
                self.assertEquals(globalMins[i,j], baseArray[i,j])

    def testMinAllDouble(self):
        "Test Epetra.PyComm MinAll method for doubles"
        baseArray   = arange(4,dtype='d')
        partialMins = array(baseArray) * (self.comm.MyPID()+1)
        globalMins  = self.comm.MinAll(partialMins)
        for i in range(len(baseArray)):
            self.assertEquals(globalMins[i], baseArray[i])

    def testMinAllDouble2D(self):
        "Test Epetra.PyComm MinAll method for 2D double array"
        baseArray    = arange(30,dtype="d")
        baseArray.shape = (6,5)
        partialMins = baseArray * (self.comm.MyPID()+1)
        globalMins  = self.comm.MinAll(partialMins)
        for i in range(6):
            for j in range(5):
                self.assertEquals(globalMins[i,j], baseArray[i,j])

    def testMinAllDoubleNoncontiguous(self):
        "Test Epetra.PyComm MinAll method with noncontiguous double array"
        baseArray   = arange(36,dtype='d')
        baseArray.shape = (6,6)
        baseArray    = baseArray[1:4,1:4]  # Noncontiguous
        partialMins = baseArray * (self.comm.MyPID()+1)
        globalMins  = self.comm.MinAll(partialMins)
        for i in range(3):
            for j in range(3):
                self.assertEquals(globalMins[i,j], baseArray[i,j])

    def testScanSumBadArg(self):
        "Test Epetra.PyComm ScanSum method with bad arguments"
        self.assertRaises(TypeError, self.comm.ScanSum, None)

    def testScanSumBadType(self):
        "Test Epetra.PyComm ScanSum method with bad type array"
        myVals = ones((3,3),'f')  # Float type not supported by Epetra
        self.assertRaises(TypeError, self.comm.ScanSum, myVals)

    def testScanSumInt(self):
        "Test Epetra.PyComm ScanSum method for ints"
        myPIN    = self.comm.MyPID() + 1
        myVals   = arange(3,dtype='i')
        scanSums = self.comm.ScanSum(myVals)
        for i in range(len(myVals)):
            self.assertEquals(scanSums[i], myPIN*myVals[i])

    def testScanSumInt2D(self):
        "Test Epetra.PyComm ScanSum method for 2D int array"
        myPIN    = self.comm.MyPID() + 1
        myVals   = arange(12,dtype='i')
        myVals.shape = (3,4)
        scanSums = self.comm.ScanSum(myVals)
        for i in range(3):
            for j in range(4):
                self.assertEquals(scanSums[i,j], myPIN*myVals[i,j])

    def testScanSumIntNoncontiguous(self):
        "Test Epetra.PyComm ScanSum method with noncontiguous int array"
        myPIN    = self.comm.MyPID() + 1
        baseArray = arange(15,dtype='i')
        baseArray.shape = (3,5)
        myVals    = baseArray[1:2,1:4]  # Noncontiguous
        scanSums  = self.comm.ScanSum(myVals)
        for i in range(1):
            for j in range(3):
                self.assertEquals(scanSums[i,j], myPIN*myVals[i,j])

    def testScanSumLong(self):
        "Test Epetra.PyComm ScanSum method for longs"
        myPIN    = self.comm.MyPID() + 1
        myVals   = arange(3)
        scanSums = self.comm.ScanSum(myVals)
        for i in range(len(myVals)):
            self.assertEquals(scanSums[i], myPIN*myVals[i])

    def testScanSumLong2D(self):
        "Test Epetra.PyComm ScanSum method for 2D long array"
        myPIN    = self.comm.MyPID() + 1
        myVals   = arange(12)
        myVals.shape = (3,4)
        scanSums = self.comm.ScanSum(myVals)
        for i in range(3):
            for j in range(4):
                self.assertEquals(scanSums[i,j], myPIN*myVals[i,j])

    def testScanSumLongNoncontiguous(self):
        "Test Epetra.PyComm ScanSum method with noncontiguous long array"
        myPIN    = self.comm.MyPID() + 1
        baseArray = arange(15)
        baseArray.shape = (3,5)
        myVals    = baseArray[1:2,1:4]  # Noncontiguous
        scanSums  = self.comm.ScanSum(myVals)
        for i in range(1):
            for j in range(3):
                self.assertEquals(scanSums[i,j], myPIN*myVals[i,j])

    def testScanSumDouble(self):
        "Test Epetra.PyComm ScanSum method for doubles"
        myPIN    = self.comm.MyPID() + 1
        myVals   = arange(4,dtype='d')
        scanSums = self.comm.ScanSum(myVals)
        for i in range(len(myVals)):
            self.assertEquals(scanSums[i], myPIN*myVals[i])

    def testScanSumDouble2D(self):
        "Test Epetra.PyComm ScanSum method for 2D double array"
        myPIN    = self.comm.MyPID() + 1
        myVals   = arange(15,dtype='d')
        myVals.shape = (5,3)
        scanSums = self.comm.ScanSum(myVals)
        for i in range(5):
            for j in range(3):
                self.assertEquals(scanSums[i,j], myPIN*myVals[i,j])

    def testScanSumDoubleNoncontiguous(self):
        "Test Epetra.PyComm ScanSum method with noncontiguous double array"
        myPIN    = self.comm.MyPID() + 1
        baseArray = arange(20,dtype='d')
        baseArray.shape = (5,4)
        myVals    = baseArray[1:4,1:3]  # Noncontiguous
        scanSums  = self.comm.ScanSum(myVals)
        for i in range(3):
            for j in range(2):
                self.assertEquals(scanSums[i,j], myPIN*myVals[i,j])

##########################################################################

if __name__ == "__main__":

    # Create the test suite object
    suite = unittest.TestSuite()

    # Add the test cases to the test suite
    suite.addTest(unittest.makeSuite(EpetraPyCommTestCase ))

    # Create a communicator
    comm    = Epetra.PyComm()
    iAmRoot = comm.MyPID() == 0

    # Run the test suite
    if iAmRoot: print >>sys.stderr, \
          "\n*******************\nTesting Epetra.Comm\n*******************\n"
    verbosity = options.verbosity * int(iAmRoot)
    result = unittest.TextTestRunner(verbosity=verbosity).run(suite)

    # Exit with a code that indicates the total number of errors and failures
    errsPlusFails = comm.SumAll(len(result.errors) + len(result.failures))
    if errsPlusFails == 0 and iAmRoot: print "End Result: TEST PASSED"
    sys.exit(errsPlusFails)
