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
try:
    import setpath
    import Epetra
except ImportError:
    from PyTrilinos import Epetra
    print "Using system-installed Epetra"

import sys
import unittest
from   Numeric    import *

##########################################################################

class EpetraObjectTestCase(unittest.TestCase):
    "TestCase for Epetra_Objects"

    def setUp(self):
        self.object = Epetra.Object()

    def testLabel(self):
        "Test Epetra.Object Label method"
        self.assertEqual(self.object.Label(), 'Epetra::Object')

    def testSetLabel(self):
        "Test Epetra.Object SetLabel method"
        label = 'New Label'
        self.object.SetLabel(label)
        self.assertEqual(self.object.Label(), label)

    def testGetTracebackMode(self):
        "Test Epetra.Object GetTracebackMode method"
        self.assertEqual(self.object.GetTracebackMode(), 1)

    def testSetTracebackMode(self):
        "Test Epetra.Object SetTracebackMode method"
        tracebackMode = 2
        self.object.SetTracebackMode(tracebackMode)
        self.assertEqual(self.object.GetTracebackMode(), tracebackMode)

##########################################################################

if __name__ == "__main__":

    # Create the test suite object
    suite = unittest.TestSuite()

    # Add the test cases to the test suite
    suite.addTest(unittest.makeSuite(EpetraObjectTestCase))

    # Run the test suite
    print >>sys.stderr, \
          "\n*********************\nTesting Epetra.Object\n*********************\n"
    unittest.TextTestRunner(verbosity=2).run(suite)
