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
import numpy
from   optparse import *
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
    import Teuchos
else:
    try:
        import setpath
        if options.boost: setpath.setpath("src-boost")
        else:             setpath.setpath()
        import Teuchos
    except ImportError:
        from PyTrilinos import Teuchos
        print >>sys.stderr, "Using system-installed Teuchos"

####################################################################

class TeuchosScalarTraitsFloatTestCase(unittest.TestCase):
    "TestCase class for Teuchos ScalarTraitsFloat class"

    def setUp(self):
        self.traits = Teuchos.ScalarTraitsFloat()

    def testConstructor(self):
        "Test Teuchos.ScalarTraitsFloat constructor"
        self.failUnless(isinstance(self.traits, Teuchos.ScalarTraitsFloat))

    def testEps(self):
        "Test Teuchos.ScalarTraitsFloat eps() method"
        self.failUnless(self.traits.eps() < 1.0e-6)

    def testSfmin(self):
        "Test Teuchos.ScalarTraitsFloat sfmin() method"
        self.failUnless(self.traits.sfmin() < 1.0e-30)

    def testBase(self):
        "Test Teuchos.ScalarTraitsFloat base() method"
        self.failUnless(self.traits.base() in (2.0,))

    def testPrec(self):
        "Test Teuchos.ScalarTraitsFloat prec() method"
        self.failUnless(self.traits.prec() < 1.0e-6)

    def testT(self):
        "Test Teuchos.ScalarTraitsFloat t() method"
        self.failUnless(self.traits.t() > 20)

    def testRnd(self):
        "Test Teuchos.ScalarTraitsFloat rnd() method"
        self.failUnless(self.traits.rnd() in (0.0, 1.0))

    def testEmin(self):
        "Test Teuchos.ScalarTraitsFloat emin() method"
        self.failUnless(self.traits.emin() < -100)

    def testRmin(self):
        "Test Teuchos.ScalarTraitsFloat rmin() method"
        self.failUnless(self.traits.rmin() < 1.0e-30)

    def testEmax(self):
        "Test Teuchos.ScalarTraitsFloat emax() method"
        self.failUnless(self.traits.emax() > 100)

    def testRmax(self):
        "Test Teuchos.ScalarTraitsFloat rmax() method"
        self.failUnless(self.traits.rmax() > 1.0e30)

    def testMagnitude(self):
        "Test Teuchos.ScalarTraitsFloat magnitude() method"
        self.assertEquals(self.traits.magnitude(10), 10)

    def testZero(self):
        "Test Teuchos.ScalarTraitsFloat zero() method"
        self.assertEquals(self.traits.zero(), 0)

    def testOne(self):
        "Test Teuchos.ScalarTraitsFloat one() method"
        self.assertEquals(self.traits.one(), 1)

    def testReal(self):
        "Test Teuchos.ScalarTraitsFloat real() method"
        self.assertEquals(self.traits.real(3), 3)

    def testImag(self):
        "Test Teuchos.ScalarTraitsFloat imag() method"
        self.assertEquals(self.traits.imag(2.718), 0)

    def testConjugate(self):
        "Test Teuchos.ScalarTraitsFloat conjugate() method"
        self.failUnless(self.traits.conjugate(1), 1)

    def testNan(self):
        "Test Teuchos.ScalarTraitsFloat nan() method"
        self.assertEquals(str(self.traits.nan()), "nan")

    def testIsnaninfNan(self):
        "Test Teuchos.ScalarTraitsFloat isnaninf() method for NaN"
        self.failUnless(self.traits.isnaninf(numpy.nan))

    def testIsnaninfNum(self):
        "Test Teuchos.ScalarTraitsFloat isnaninf() method for number"
        self.failIf(self.traits.isnaninf(3))

    def testSeedrandom(self):
        "Test Teuchos.ScalarTraitsFloat seedrandom() method"
        self.traits.seedrandom(1)
        # Test passes if no exception raised

    def testRandom(self):
        "Test Teuchos.ScalarTraitsFloat random() method"
        r = self.traits.random()
        self.failUnless(r > -1)
        self.failUnless(r <  1)

    def testName(self):
        "Test Teuchos.ScalarTraitsFloat name() method"
        self.assertEquals(self.traits.name(), "float")

    def testSquareroot(self):
        "Test Teuchos.ScalarTraitsFloat squareroot() method"
        self.failUnlessEqual(self.traits.squareroot(25), 5)

    def testPow(self):
        "Test Teuchos.ScalarTraitsFloat pow() method"
        self.failUnless(self.traits.pow(2,5), 32)

    def testIsComplex(self):
        "Test Teuchos.ScalarTraitsFloat isComplex attribute"
        self.failIf(self.traits.isComplex)

    def testIsComparable(self):
        "Test Teuchos.ScalarTraitsFloat isComparable attribute"
        self.failUnless(self.traits.isComparable)

    def testHasMachineParameters(self):
        "Test Teuchos.ScalarTraitsFloat hasMachineParameters attribute"
        self.failUnless(self.traits.hasMachineParameters)

####################################################################

class TeuchosScalarTraitsDoubleTestCase(unittest.TestCase):
    "TestCase class for Teuchos ScalarTraitsDouble class"

    def setUp(self):
        self.traits = Teuchos.ScalarTraitsDouble()

    def testConstructor(self):
        "Test Teuchos.ScalarTraitsDouble constructor"
        self.failUnless(isinstance(self.traits, Teuchos.ScalarTraitsDouble))

    def testEps(self):
        "Test Teuchos.ScalarTraitsDouble eps() method"
        self.failUnless(self.traits.eps() < 1.0e-15)

    def testSfmin(self):
        "Test Teuchos.ScalarTraitsDouble sfmin() method"
        self.failUnless(self.traits.sfmin() < 1.0e-300)

    def testBase(self):
        "Test Teuchos.ScalarTraitsDouble base() method"
        self.failUnless(self.traits.base() in (2.0,))

    def testPrec(self):
        "Test Teuchos.ScalarTraitsDouble prec() method"
        self.failUnless(self.traits.prec() < 1.0e-15)

    def testT(self):
        "Test Teuchos.ScalarTraitsDouble t() method"
        self.failUnless(self.traits.t() > 50)

    def testRnd(self):
        "Test Teuchos.ScalarTraitsDouble rnd() method"
        self.failUnless(self.traits.rnd() in (0.0, 1.0))

    def testEmin(self):
        "Test Teuchos.ScalarTraitsDouble emin() method"
        self.failUnless(self.traits.emin() < -1000)

    def testRmin(self):
        "Test Teuchos.ScalarTraitsDouble rmin() method"
        self.failUnless(self.traits.rmin() < 1.0e-300)

    def testEmax(self):
        "Test Teuchos.ScalarTraitsDouble emax() method"
        self.failUnless(self.traits.emax() > 1000)

    def testRmax(self):
        "Test Teuchos.ScalarTraitsDouble rmax() method"
        self.failUnless(self.traits.rmax() > 1.0e300)

    def testMagnitude(self):
        "Test Teuchos.ScalarTraitsDouble magnitude() method"
        self.assertEquals(self.traits.magnitude(10), 10)

    def testZero(self):
        "Test Teuchos.ScalarTraitsDouble zero() method"
        self.assertEquals(self.traits.zero(), 0)

    def testOne(self):
        "Test Teuchos.ScalarTraitsDouble one() method"
        self.assertEquals(self.traits.one(), 1)

    def testReal(self):
        "Test Teuchos.ScalarTraitsDouble real() method"
        self.assertEquals(self.traits.real(3.14), 3.14)

    def testImag(self):
        "Test Teuchos.ScalarTraitsDouble imag() method"
        self.assertEquals(self.traits.imag(2.718), 0)

    def testConjugate(self):
        "Test Teuchos.ScalarTraitsDouble conjugate() method"
        self.failUnless(self.traits.conjugate(1), 1)

    def testNan(self):
        "Test Teuchos.ScalarTraitsDouble nan() method"
        self.assertEquals(str(self.traits.nan()), "nan")

    def testIsnaninfNan(self):
        "Test Teuchos.ScalarTraitsDouble isnaninf() method for NaN"
        self.failUnless(self.traits.isnaninf(numpy.nan))

    def testIsnaninfInf(self):
        "Test Teuchos.ScalarTraitsDouble isnaninf() method for Inf"
        self.failUnless(self.traits.isnaninf(numpy.inf))

    def testIsnaninfNum(self):
        "Test Teuchos.ScalarTraitsDouble isnaninf() method for number"
        self.failIf(self.traits.isnaninf(3))

    def testSeedrandom(self):
        "Test Teuchos.ScalarTraitsDouble seedrandom() method"
        self.traits.seedrandom(1)
        # Test passes if no exception raised

    def testRandom(self):
        "Test Teuchos.ScalarTraitsDouble random() method"
        r = self.traits.random()
        self.failUnless(r > -1)
        self.failUnless(r <  1)

    def testName(self):
        "Test Teuchos.ScalarTraitsDouble name() method"
        self.assertEquals(self.traits.name(), "double")

    def testSquareroot(self):
        "Test Teuchos.ScalarTraitsDouble squareroot() method"
        self.failUnlessEqual(self.traits.squareroot(25), 5)

    def testPow(self):
        "Test Teuchos.ScalarTraitsDouble pow() method"
        self.failUnless(self.traits.pow(2,5), 32)

    def testIsComplex(self):
        "Test Teuchos.ScalarTraitsDouble isComplex attribute"
        self.failIf(self.traits.isComplex)

    def testIsComparable(self):
        "Test Teuchos.ScalarTraitsDouble isComparable attribute"
        self.failUnless(self.traits.isComparable)

    def testHasMachineParameters(self):
        "Test Teuchos.ScalarTraitsDouble hasMachineParameters attribute"
        self.failUnless(self.traits.hasMachineParameters)

####################################################################

class TeuchosScalarTraitsTestCase(unittest.TestCase):
    "TestCase class for Teuchos ScalarTraits factory"

    def testFloat(self):
        "Test Teuchos.ScalarTraits factory for float"
        traits = Teuchos.ScalarTraits('f')
        self.failUnless(isinstance(traits, Teuchos.ScalarTraitsFloat))

    def testDouble(self):
        "Test Teuchos.ScalarTraits factory for double"
        traits = Teuchos.ScalarTraits('d')
        self.failUnless(isinstance(traits, Teuchos.ScalarTraitsDouble))

    def testBad(self):
        "Test Teuchos.ScalarTraits factory for bad argument"
        self.assertRaises(NotImplementedError, Teuchos.ScalarTraits, 'a')

####################################################################

if __name__ == "__main__":

    # Create the test suite object
    suite = unittest.TestSuite()

    # Add the test cases to the test suite
    suite.addTest(unittest.makeSuite(TeuchosScalarTraitsFloatTestCase ))
    suite.addTest(unittest.makeSuite(TeuchosScalarTraitsDoubleTestCase))
    suite.addTest(unittest.makeSuite(TeuchosScalarTraitsTestCase      ))

    iAmRoot = True

    # Run the test suite
    if iAmRoot: print >>sys.stderr, \
       "\n****************************\nTesting Teuchos.ScalarTraits\n" + \
       "****************************\n"
    verbosity = options.verbosity * int(iAmRoot)
    result = unittest.TextTestRunner(verbosity=verbosity).run(suite)

    # Exit with a code that indicates the total number of errors and failures
    errsPlusFails = len(result.errors) + len(result.failures)
    if errsPlusFails == 0 and iAmRoot: print "End Result: TEST PASSED"
    sys.exit(errsPlusFails)
