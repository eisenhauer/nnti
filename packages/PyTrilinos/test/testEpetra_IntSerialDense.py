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

class EpetraIntSerialDenseMatrixTestCase(unittest.TestCase):
    "TestCase class for Epetra IntSerialDense Matrices"

    def setUp(self):
        self.comm  = Epetra.PyComm()
        self.rows  = 3
        self.cols  = 4
        self.array = [[4, 3, 2, 1],
                      [3, 4, 3, 2],
                      [2, 3, 4, 3]]

    def tearDown(self):
        self.comm.Barrier()

    def testConstructor00(self):
        "Test Epetra.IntSerialDenseMatrix default constructor"
        isdm = Epetra.IntSerialDenseMatrix()
        self.assertEqual(isdm.CV(), Epetra.Copy)
        self.assertEqual(isdm.M() , 0          )
        self.assertEqual(isdm.N() , 0          )

    def testConstructor01(self):
        "Test Epetra.IntSerialDenseMatrix (int,int) constructor"
        isdm = Epetra.IntSerialDenseMatrix(self.rows,self.cols)
        self.assertEqual(isdm.CV(), Epetra.Copy)
        self.assertEqual(isdm.M() , self.rows  )
        self.assertEqual(isdm.N() , self.cols  )
        self.failUnless((isdm == zeros((self.rows,self.cols),'i')).all())

    def testConstructor02(self):
        "Test Epetra.IntSerialDenseMatrix (int,int) constructor, negative rows"
        self.assertRaises(Epetra.Error, Epetra.IntSerialDenseMatrix, -self.rows,
                          self.cols)

    def testConstructor03(self):
        "Test Epetra.IntSerialDenseMatrix (int,int) constructor, negative columns"
        self.assertRaises(Epetra.Error, Epetra.IntSerialDenseMatrix, self.rows,
                          -self.cols)

    def testConstructor04(self):
        "Test Epetra.IntSerialDenseMatrix (1D-array) constructor"
        self.assertRaises((TypeError,ValueError), Epetra.IntSerialDenseMatrix,
                          [0,1,2,3])

    def testConstructor05(self):
        "Test Epetra.IntSerialDenseMatrix (2D-array) constructor"
        isdm = Epetra.IntSerialDenseMatrix(self.array)
        self.assertEqual(isdm.CV(), Epetra.View)
        self.assertEqual(isdm.M(), self.rows)
        self.assertEqual(isdm.N(), self.cols)

    def testConstructor06(self):
        "Test Epetra.IntSerialDenseMatrix (3D-array) constructor"
        myArray = [self.array, self.array]
        self.assertRaises((TypeError,ValueError), Epetra.IntSerialDenseMatrix,
                          myArray)

    def testConstructor07(self):
        "Test Epetra.IntSerialDenseMatrix (bad-type-array) constructor"
        myArray = [ [0, 1.2], ["e", "pi"] ]
        self.assertRaises((TypeError,ValueError), Epetra.IntSerialDenseMatrix,
                          myArray)

    def testConstructor08(self):
        "Test Epetra.IntSerialDenseMatrix copy constructor for default"
        isdm1 = Epetra.IntSerialDenseMatrix()
        isdm2 = Epetra.IntSerialDenseMatrix(isdm1)
        self.assertEqual(isdm2.CV() ,isdm1.CV() )
        self.assertEqual(isdm2.M()  ,isdm1.M()  )
        self.assertEqual(isdm2.N()  ,isdm1.N()  )
        self.assertEqual(isdm2.LDA(),isdm1.LDA())

    def testConstructor09(self):
        "Test Epetra.IntSerialDenseMatrix copy constructor for (int,int)"
        isdm1 = Epetra.IntSerialDenseMatrix(self.rows,self.cols)
        isdm1.Random()
        isdm2 = Epetra.IntSerialDenseMatrix(isdm1)
        self.assertEqual(isdm2.CV() ,isdm1.CV() )
        self.assertEqual(isdm2.M()  ,isdm1.M()  )
        self.assertEqual(isdm2.N()  ,isdm1.N()  )
        self.assertEqual(isdm2.LDA(),isdm1.LDA())
        self.failUnless((isdm1 == isdm2).all())

    def testConstructor10(self):
        "Test Epetra.IntSerialDenseMatrix copy constructor for (2D-array)"
        isdm1 = Epetra.IntSerialDenseMatrix(self.array)
        isdm2 = Epetra.IntSerialDenseMatrix(isdm1)
        self.assertEqual(isdm2.CV() ,isdm1.CV() )
        self.assertEqual(isdm2.M()  ,isdm1.M()  )
        self.assertEqual(isdm2.N()  ,isdm1.N()  )
        self.assertEqual(isdm2.LDA(),isdm1.LDA())
        self.failUnless((isdm1 == isdm2).all())

    def testShape1(self):
        "Test Epetra.IntSerialDenseMatrix Shape method"
        isdm   = Epetra.IntSerialDenseMatrix(self.rows,self.cols)
        isdm.Random()
        result = isdm.Shape(self.cols,self.rows)
        self.failUnless((isdm == zeros((self.cols,self.rows),'i')).all())

    def testShape2(self):
        "Test Epetra.IntSerialDenseMatrix Shape method, negative rows"
        isdm = Epetra.IntSerialDenseMatrix()
        self.assertRaises(ValueError, isdm.Shape, -self.rows, self.cols)

    def testShape3(self):
        "Test Epetra.IntSerialDenseMatrix Shape method, negative columns"
        isdm = Epetra.IntSerialDenseMatrix()
        self.assertRaises(ValueError, isdm.Shape, self.rows, -self.cols)

    def testReshape1(self):
        "Test Epetra.IntSerialDenseMatrix Reshape method to smaller"
        isdm1  = Epetra.IntSerialDenseMatrix(self.rows,self.cols)
        isdm1.Random()
        isdm2  = Epetra.IntSerialDenseMatrix(isdm1)
        result = isdm2.Reshape(self.rows-1,self.cols-1)
        self.assertEqual(result, 0)
        self.failUnless((isdm2 == isdm1[:-1,:-1]).all())

    def testReshape2(self):
        "Test Epetra.IntSerialDenseMatrix Reshape method to larger"
        isdm1  = Epetra.IntSerialDenseMatrix(self.rows,self.cols)
        isdm1.Random()
        isdm2  = Epetra.IntSerialDenseMatrix(isdm1)
        result = isdm2.Reshape(self.rows+1,self.cols+1)
        self.assertEqual(result, 0)
        self.failUnless((isdm2[:-1,:-1] == isdm1).all())

    def testReshape3(self):
        "Test Epetra.IntSerialDenseMatrix Reshape method, negative rows"
        isdm = Epetra.IntSerialDenseMatrix()
        self.assertRaises(ValueError, isdm.Reshape, -self.rows, self.cols)

    def testReshape4(self):
        "Test Epetra.IntSerialDenseMatrix Reshape method, negative columns"
        isdm = Epetra.IntSerialDenseMatrix()
        self.assertRaises(ValueError, isdm.Reshape, self.rows, -self.cols)

    def testOneNorm(self):
        "Test Epetra.IntSerialDenseMatrix OneNorm method"
        a    = array(self.array)
        isdm = Epetra.IntSerialDenseMatrix(self.array)
        self.assertEqual(isdm.OneNorm(), max(sum(abs(a),axis=0)))

    def testInfNorm(self):
        "Test Epetra.IntSerialDenseMatrix InfNorm method"
        a    = array(self.array)
        isdm = Epetra.IntSerialDenseMatrix(self.array)
        self.assertEqual(isdm.InfNorm(), max(sum(abs(a),axis=1)))

    def testParentheses(self):
        "Test Epetra.IntSerialDenseMatrix __call__ method"
        isdm = Epetra.IntSerialDenseMatrix(self.array)
        for i in range(isdm.M()):
            for j in range(isdm.N()):
                self.assertEqual(isdm(i,j), self.array[i][j])

    def testGetItem1(self):
        "Test Epetra.IntSerialDenseMatrix __getitem__ method for two ints"
        isdm = Epetra.IntSerialDenseMatrix(self.array)
        for i in range(isdm.M()):
            for j in range(isdm.N()):
                self.assertEqual(isdm[i,j], self.array[i][j])

    def testGetItem2(self):
        "Test Epetra.IntSerialDenseMatrix __getitem__ method for int and slice"
        isdm = Epetra.IntSerialDenseMatrix(self.array)
        for i in range(isdm.M()):
            self.failUnless((isdm[i,:] == self.array[i]).all())

    def testGetItem3(self):
        "Test Epetra.IntSerialDenseMatrix __getitem__ method for two slices"
        isdm = Epetra.IntSerialDenseMatrix(self.array)
        self.failUnless((isdm[:,:] == self.array).all())

    def testSetitem1(self):
        "Test Epetra.IntSerialDenseMatrix __setitem__ method for two ints"
        isdm = Epetra.IntSerialDenseMatrix(self.rows,self.cols)
        self.failUnless((isdm == zeros((self.rows,self.cols),'i')).all())
        for i in range(isdm.M()):
            for j in range(isdm.N()):
                value = i * isdm.N() + j
                isdm[i,j] = value
                self.assertEqual(isdm[i,j], value)

    def testSetitem2(self):
        "Test Epetra.IntSerialDenseMatrix __setitem__ method for int and slice"
        isdm = Epetra.IntSerialDenseMatrix(self.rows,self.cols)
        self.failUnless((isdm == zeros((self.rows,self.cols),'i')).all())
        for i in range(isdm.M()):
            isdm[i,:] = self.array[i]
            self.failUnless((isdm[i,:] == self.array[i]).all())

    def testSetitem3(self):
        "Test Epetra.IntSerialDenseMatrix __setitem__ method for two slices"
        isdm = Epetra.IntSerialDenseMatrix(self.rows,self.cols)
        self.failUnless((isdm == zeros((self.rows,self.cols),'i')).all())
        isdm[:,:] = self.array
        self.failUnless((isdm == self.array).all())

    def testRandom(self):
        "Test Epetra.IntSerialDenseMatrix Random method"
        isdm = Epetra.IntSerialDenseMatrix(self.rows,self.cols)
        self.failUnless((isdm == zeros((self.rows,self.cols),'i')).all())
        result = isdm.Random()
        self.assertEqual(result, 0)
        count = 0
        for i in range(self.rows):
            for j in range(self.cols):
                if isdm[i,j] == 0: count += 1
        self.failUnless(count < self.rows*self.cols)

    def testM(self):
        "Test Epetra.IntSerialDenseMatrix M method"
        for numRows in range(4,10):
            for numCols in range(4,10):
                isdm = Epetra.IntSerialDenseMatrix(numRows,numCols)
                self.assertEqual(isdm.M(), numRows)

    def testN(self):
        "Test Epetra.IntSerialDenseMatrix N method"
        for numRows in range(4,10):
            for numCols in range(4,10):
                isdm = Epetra.IntSerialDenseMatrix(numRows,numCols)
                self.assertEqual(isdm.N(), numCols)

    def testA1(self):
        "Test Epetra.IntSerialDenseMatrix A method for default"
        isdm = Epetra.IntSerialDenseMatrix()
        self.failUnless((isdm.A() == isdm.array).all())

    def testA2(self):
        "Test Epetra.IntSerialDenseMatrix A method for (int,int)"
        isdm = Epetra.IntSerialDenseMatrix(self.rows,self.cols)
        result = isdm.Random()
        self.failUnless((isdm.A() == isdm.array).all())

    def testA3(self):
        "Test Epetra.IntSerialDenseMatrix A method for (2D-array)"
        isdm = Epetra.IntSerialDenseMatrix(self.array)
        self.failUnless((isdm.A() == isdm.array).all())

    def testArray(self):
        "Test Epetra.IntSerialDenseMatrix array attribute"
        isdm = Epetra.IntSerialDenseMatrix()
        self.assertRaises(AttributeError, isdm.__setattr__, 'array', None)

    def testLDA(self):
        "Test Epetra.IntSerialDenseMatrix LDA method"
        for numRows in range(4,10):
            for numCols in range(4,10):
                isdm = Epetra.IntSerialDenseMatrix(numRows,numCols)
                self.assertEqual(isdm.LDA(),isdm.M())

    def testPrint(self):
        "Test Epetra.IntSerialDenseMatrix Print method"
        myPID    = self.comm.MyPID()
        filename = "testIntSerialDense%d.dat" % myPID
        f        = open(filename, "w")
        isdm = Epetra.IntSerialDenseMatrix(self.array)
        isdm.Print(f)
        f.close()
        self.assertEqual(open(filename, "r").read(),
                         "Data access mode: View\n"
                         "A_Copied: no\n"
                         "Rows(M): 3\n"
                         "Columns(N): 4\n"
                         "LDA: 3\n"
                         "4 3 2 1 \n"
                         "3 4 3 2 \n"
                         "2 3 4 3 \n")

    def testStr(self):
        "Test Epetra.IntSerialDenseMatrix __str__ method"
        isdm = Epetra.IntSerialDenseMatrix(self.array)
        self.assertEqual(isdm.__str__(),
                         "[[4 3 2 1]\n"
                         " [3 4 3 2]\n"
                         " [2 3 4 3]]")

##########################################################################

class EpetraIntSerialDenseVectorTestCase(unittest.TestCase):
    "TestCase class for Epetra IntSerialDense Vectors"

    def setUp(self):
        self.comm = Epetra.PyComm()
        self.size = 4
        self.list = [0, -1, 2, -3]
        # Query whether Copy objects are actually Copy objects.  For older
        # versions of SWIG, they are View objects, but newer versions of SWIG
        # get it right
        isdv = Epetra.IntSerialDenseVector(self.size)
        self.CV = isdv.CV()
        if self.CV == Epetra.View:
            self.CVStr = "View"
            self.YNStr = "no"
        else:
            self.CVStr = "Copy"
            self.YNStr = "yes"

    def tearDown(self):
        self.comm.Barrier()

    def testConstructor0(self):
        "Test Epetra.IntSerialDenseVector default constructor"
        isdv = Epetra.IntSerialDenseVector()
        self.assertEqual(isdv.CV(), Epetra.Copy)
        self.assertEqual(isdv.Length(), 0)

    def testConstructor1(self):
        "Test Epetra.IntSerialDenseVector (int) constructor"
        isdv = Epetra.IntSerialDenseVector(self.size)
        self.assertEqual(isdv.CV(), self.CV)
        self.assertEqual(isdv.Length(), self.size)

    def testConstructor2(self):
        "Test Epetra.IntSerialDenseVector (1D-list) constructor"
        isdv = Epetra.IntSerialDenseVector(self.list)
        self.assertEqual(isdv.CV(), Epetra.View)
        self.assertEqual(isdv.Length(), len(self.list))
        self.failUnless((isdv == self.list).all())

    def testConstructor3(self):
        "Test Epetra.IntSerialDenseVector (2D-list) constructor"
        list = [[0, -1, 2], [-3, 4, -5]]
        isdv = Epetra.IntSerialDenseVector(list)
        self.assertEqual(isdv.CV(), Epetra.View)
        self.assertEqual(isdv.Length(), len(list)*len(list[0]))
        self.failUnless((isdv == list).all())

    def testConstructor4(self):
        "Test Epetra.IntSerialDenseVector (bad-list) constructor"
        list = [0,1.0,"e","pi"]
        self.assertRaises((TypeError,ValueError),Epetra.IntSerialDenseVector,
                          list)

    def testConstructor5(self):
        "Test Epetra.IntSerialDenseVector copy constructor for default"
        isdv1 = Epetra.IntSerialDenseVector()
        isdv2 = Epetra.IntSerialDenseVector(isdv1)
        self.assertEqual(isdv2.CV()    , isdv1.CV()    )
        self.assertEqual(isdv1.Length(), isdv2.Length())

    def testConstructor6(self):
        "Test Epetra.IntSerialDenseVector copy constructor for (int)"
        isdv1 = Epetra.IntSerialDenseVector(self.size)
        isdv2 = Epetra.IntSerialDenseVector(isdv1)
        self.assertEqual(isdv2.CV()    , isdv1.CV()    )
        self.assertEqual(isdv1.Length(), isdv2.Length())
        self.failUnless((isdv1 == isdv2).all())

    def testConstructor7(self):
        "Test Epetra.IntSerialDenseVector copy constructor for (list)"
        isdv1 = Epetra.IntSerialDenseVector(self.list)
        isdv2 = Epetra.IntSerialDenseVector(isdv1)
        self.assertEqual(isdv2.CV()    , isdv1.CV()    )
        self.assertEqual(isdv1.Length(), isdv2.Length())
        self.failUnless((isdv1 == isdv2).all())

    def testPrint(self):
        "Test Epetra.IntSerialDenseVector Print method"
        isdv     = Epetra.IntSerialDenseVector(self.size)
        isdv[:]  = 0
        filename = "testIntSerialDense%d.dat" % self.comm.MyPID()
        f = open(filename, "w")
        isdv.Print(f)
        f.close()
        out = "Data access mode: %s\nA_Copied: %s\nLength(M): %d\n" % \
              (self.CVStr,self.YNStr,self.size) + self.size * "0 " + "\n"
        f = open(filename, "r")
        self.assertEqual(f.read(), out)
        f.close()

    def testStr(self):
        "Test Epetra.IntSerialDenseVector __str__ method"
        isdv = Epetra.IntSerialDenseVector(self.size)
        isdv[:] = 0
        out = "[" + self.size * "0 "
        out = out [:-1] + "]"
        self.assertEquals(str(isdv), out)

    def testSize(self):
        "Test Epetra.IntSerialDenseVector Size method"
        isdv = Epetra.IntSerialDenseVector()
        result = isdv.Size(3*self.size)
        self.assertEqual(result, 0)
        self.assertEqual(isdv.Length(), 3*self.size)
        self.assertEqual(isdv.Length(), len(isdv))
        self.failUnless((isdv == 0).all())

    def testResize1(self):
        "Test Epetra.IntSerialDenseVector Resize method to smaller"
        isdv = Epetra.IntSerialDenseVector(3*self.size)
        self.assertEqual(isdv.Length(), 3*self.size)
        self.assertEqual(isdv.Length(), len(isdv))
        isdv[:] = range(len(isdv))
        result = isdv.Resize(self.size)
        self.assertEqual(result, 0)
        self.assertEqual(isdv.Length(), self.size)
        self.assertEqual(isdv.Length(), len(isdv))
        for i in range(len(isdv)):
            self.assertEqual(isdv[i], i)

    def testResize2(self):
        "Test Epetra.IntSerialDenseVector Resize method to larger"
        isdv = Epetra.IntSerialDenseVector(self.size)
        self.assertEqual(isdv.Length(), self.size)
        self.assertEqual(isdv.Length(), len(isdv))
        isdv[:] = range(len(isdv))
        isdv.Resize(3*self.size)
        self.assertEqual(isdv.Length(), 3*self.size)
        self.assertEqual(isdv.Length(), len(isdv))
        for i in range(len(isdv)):
            if i < self.size:
                self.assertEqual(isdv[i], i)
            else:
                self.assertEqual(isdv[i], 0.0)

    def testGetitem1(self):
        "Test Epetra.IntSerialDenseVector __getitem__ method"
        isdv = Epetra.IntSerialDenseVector(self.list)
        for i in range(len(self.list)):
            self.assertEqual(isdv[i],self.list[i])

    def testGetitem2(self):
        "Test Epetra.IntSerialDenseVector __call__ method"
        isdv = Epetra.IntSerialDenseVector(self.list)
        for i in range(len(self.list)):
            self.assertEqual(isdv(i),self.list[i])

    def testSetitem(self):
        "Test Epetra.IntSerialDenseVector __setitem__ method"
        isdv = Epetra.IntSerialDenseVector(len(self.list))
        isdv[:] = 0
        self.failUnless((isdv == 0).all())
        for i in range(len(isdv)):
            isdv[i] = self.list[i]
        self.failUnless((isdv == self.list).all())

    def testIndexErrors(self):
        "Test Epetra.IntSerialDenseVector index errors"
        isdv = Epetra.IntSerialDenseVector(self.size)
        self.assertRaises(TypeError, isdv.__getitem__, 0,1)
        self.assertRaises(TypeError, isdv.__setitem__, 0,1,3.14)

    def testRandom(self):
        "Test Epetra.IntSerialDenseVector Random method"
        isdv    = Epetra.IntSerialDenseVector(self.size)
        isdv[:] = 0
        self.failUnless((isdv == 0).all())
        result = isdv.Random()
        self.assertEqual(result, 0)
        maxInt = 2**31 - 1
        for i in range(self.size):
            self.failUnless(isdv[i] >= 0     )
            self.failUnless(isdv[i] <= maxInt)

    def testLength(self):
        "Test Epetra.IntSerialDenseVector Length method"
        isdv = Epetra.IntSerialDenseVector()
        self.assertEqual(isdv.Length(),0)
        self.assertEqual(isdv.Length(),len(isdv))
        isdv.Size(self.size)
        self.assertEqual(isdv.Length(),self.size)
        self.assertEqual(isdv.Length(),len(isdv))
        isdv.Resize(2*self.size)
        self.assertEqual(isdv.Length(),2*self.size)
        self.assertEqual(isdv.Length(),len(isdv))
        isdv.Resize(self.size)
        self.assertEqual(isdv.Length(),self.size)
        self.assertEqual(isdv.Length(),len(isdv))

    def testValues(self):
        "Test Epetra.IntSerialDenseVector Values method"
        isdv  = Epetra.IntSerialDenseVector(self.list)
        vals = isdv.Values()
        self.assertNotEqual(type(isdv), type(vals))
        self.assertEqual(isdv.Length(), len(vals))
        for i in range(len(vals)):
            self.assertEqual(isdv[i], vals[i])

    def testInPlaceAdd(self):
        "Test Epetra.IntSerialDenseVector += operator"
        isdv = Epetra.IntSerialDenseVector(self.list)
        truthVector = (isdv == self.list)
        self.failUnless(truthVector.all())
        isdv += isdv
        for i in range(isdv.Length()):
            self.assertEqual(isdv[i],2*self.list[i])

##########################################################################

if __name__ == "__main__":

    # Create the test suite object
    suite = unittest.TestSuite()

    # Add the test cases to the test suite
    suite.addTest(unittest.makeSuite(EpetraIntSerialDenseMatrixTestCase))
    suite.addTest(unittest.makeSuite(EpetraIntSerialDenseVectorTestCase))

    # Create a communicator
    comm    = Epetra.PyComm()
    iAmRoot = comm.MyPID() == 0
    comm.SetTracebackMode(0)    # Turn off Epetra errors printed to stderr

    # Run the test suite
    if iAmRoot: print >>sys.stderr, \
       "\n*****************************" \
       "\nTesting Epetra.IntSerialDense" \
       "\n*****************************\n"
    verbosity = options.verbosity * int(iAmRoot)
    result = unittest.TextTestRunner(verbosity=verbosity).run(suite)

    # Exit with a code that indicates the total number of errors and failures
    errsPlusFails = comm.SumAll(len(result.errors) + len(result.failures))
    if errsPlusFails == 0 and iAmRoot: print "End Result: TEST PASSED"
    sys.exit(errsPlusFails)
