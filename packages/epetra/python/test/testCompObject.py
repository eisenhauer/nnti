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

from   Numeric  import *
import unittest

##########################################################################

class EpetraCompObjectTestCase(unittest.TestCase):
    "TestCase for Epetra_CompObjects"

    def setUp(self):
        self.comm       = Epetra.PyComm()
        self.FLOPS      = Epetra.FLOPS()
        self.compObject = Epetra.CompObject()
        self.compObject.SetFlopCounter(self.FLOPS)
        self.comm.Barrier()

    def tearDown(self):
        self.comm.Barrier()

    def testConstructor1(self):
        "Test Epetra.CompObject constructor"
        self.assertEqual(type(self.compObject), Epetra.CompObject)

    def testConstructor2(self):
        "Test Epetra.CompObject copy constructor"
        compObject = Epetra.CompObject(self.compObject)
        self.assertEqual(type(compObject), Epetra.CompObject)

    def testSetFlopCounter1(self):
        "Test Epetra.CompObject SetFlopCounter method w/FLOPS arg"
        flops = 4
        self.compObject.UpdateFlops(flops)   # Change the current FLOP count
        self.assertEqual(self.compObject.Flops(), flops)
        flopCounter = Epetra.FLOPS()         # New counter set to zero
        self.compObject.SetFlopCounter(flopCounter)
        self.assertEqual(self.compObject.Flops(), 0)

    def testSetFlopCounter2(self):
        "Test Epetra.CompObject SetFlopCounter method w/CompObject arg"
        flops = 17
        FLOPS = Epetra.FLOPS()
        compObject = Epetra.CompObject()
        compObject.SetFlopCounter(FLOPS)
        compObject.UpdateFlops(flops)
        self.compObject.SetFlopCounter(compObject)
        self.assertEqual(self.compObject.Flops(), flops)

    def testUnsetFlopCounter(self):
        "Test Epetra.CompObject UnsetFlopCounter method"
        flops = 2004
        self.compObject.UpdateFlops(flops)  # Change the current FLOP count
        self.compObject.UnsetFlopCounter()
        self.assertEqual(self.compObject.Flops(), 0.0)

    def testGetFlopCounter(self):
        "Test Epetra.CompObject GetFlopCounter method"
        flops = 1
        self.compObject.UpdateFlops(flops)  # Change the current FLOP count
        FLOPS = self.compObject.GetFlopCounter()
        self.assertEqual(FLOPS.Flops(), flops)

    def testResetFlops(self):
        "Test Epetra.CompObject ResetFlops method"
        flops = 15
        self.compObject.UpdateFlops(flops)  # Change the current FLOP count
        self.assertEqual(self.compObject.Flops(), flops)
        self.compObject.ResetFlops()
        self.assertEqual(self.compObject.Flops(), 0.0)

    def testFlops(self):
        "Test Epetra.CompObject Flops method"
        flops = 2000
        self.compObject.UpdateFlops(flops)
        self.assertEqual(self.compObject.Flops(), flops)

    def testUpdateFlops1(self):
        "Test Epetra.CompObject UpdateFlops method w/int arg"
        flops = 11
        self.compObject.UpdateFlops(flops)
        self.assertEqual(self.compObject.Flops(), flops)

    def testUpdateFlops2(self):
        "Test Epetra.CompObject UpdateFlops method w/float arg"
        flops = 10.0
        self.compObject.UpdateFlops(flops)
        self.assertEqual(self.compObject.Flops(), flops)

##########################################################################

if __name__ == "__main__":

    # Create the test suite object
    suite = unittest.TestSuite()

    # Add the test cases to the test suite
    suite.addTest(unittest.makeSuite(EpetraCompObjectTestCase))

    # Create a communicator
    comm    = Epetra.PyComm()
    iAmRoot = comm.MyPID() == 0

    # Run the test suite
    if iAmRoot: print >>sys.stderr, \
          "\n*************************\nTesting Epetra.CompObject\n*************************\n"
    verbosity = 2 * int(iAmRoot)
    result = unittest.TextTestRunner(verbosity=verbosity).run(suite)

    # Exit with a code that indicates the total number of errors and failures
    errsPlusFails = comm.SumAll(len(result.errors) + len(result.failures))[0]
    if errsPlusFails == 0 and iAmRoot: print "End Result: TEST PASSED"
    sys.exit(errsPlusFails)
