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
from   numpy    import *

##########################################################################

class EpetraSerialDenseMatrixTestCase(unittest.TestCase):
    "TestCase class for Epetra SerialDense Matrices"

    def setUp(self):
        self.comm  = Epetra.PyComm()
        self.size  = 4
        self.rows  = 3
        self.cols  = 4
        self.array = [[0.1, 2.3, 4.5, 6.7],
                      [6.7, 8.9, 0.1, 2.3],
                      [2.3, 4.5, 6.7, 8.9]]
        # Query whether Copy objects are actually Copy objects.  For older
        # versions of SWIG, they are View objects, but newer versions of SWIG
        # get it right
        sdm = Epetra.SerialDenseMatrix(False)
        self.CV = sdm.CV()
        if self.CV == Epetra.View:
            self.CVStr = "View"
            self.YNStr = "no"
        else:
            self.CVStr = "Copy"
            self.YNStr = "yes"

    def tearDown(self):
        self.comm.Barrier()

    def testConstructor00(self):
        "Test Epetra.SerialDenseMatrix default constructor"
        sdm = Epetra.SerialDenseMatrix()
        self.assertEqual(sdm.CV(), Epetra.Copy)
        self.assertEqual(sdm.M() , 0          )
        self.assertEqual(sdm.N() , 0          )

    def testConstructor01(self):
        "Test Epetra.SerialDenseMatrix bool constructor"
        sdm = Epetra.SerialDenseMatrix(False)
        self.assertEqual(sdm.CV(), self.CV)
        self.assertEqual(sdm.M() , 0      )
        self.assertEqual(sdm.N() , 0      )

    def testConstructor02(self):
        "Test Epetra.SerialDenseMatrix (int,int) constructor"
        sdm = Epetra.SerialDenseMatrix(self.rows,self.cols)
        self.assertEqual(sdm.CV(), self.CV  )
        self.assertEqual(sdm.M(),  self.rows)
        self.assertEqual(sdm.N(),  self.cols)

    def testConstructor03(self):
        "Test Epetra.SerialDenseMatrix (int,int) constructor, negative rows"
        self.assertRaises(Epetra.Error, Epetra.SerialDenseMatrix, -self.rows,
                          self.cols)

    def testConstructor04(self):
        "Test Epetra.SerialDenseMatrix (int,int) constructor, negative columns"
        self.assertRaises(Epetra.Error, Epetra.SerialDenseMatrix, self.rows,
                          -self.cols)

    def testConstructor05(self):
        "Test Epetra.SerialDenseMatrix (int,int,bool) constructor"
        sdm = Epetra.SerialDenseMatrix(self.rows,self.cols,False)
        self.assertEqual(sdm.CV(), Epetra.Copy)
        self.assertEqual(sdm.M() , self.rows  )
        self.assertEqual(sdm.N() , self.cols  )

    def testConstructor06(self):
        "Test Epetra.SerialDenseMatrix (1D-list) constructor"
        list = self.array[1]
        self.assertRaises(TypeError, Epetra.SerialDenseMatrix, list)

    def testConstructor07(self):
        "Test Epetra.SerialDenseMatrix (2D-list) constructor"
        sdm = Epetra.SerialDenseMatrix(self.array)
        self.assertEqual(sdm.CV(), Epetra.View       )
        self.assertEqual(sdm.M() , len(self.array)   )
        self.assertEqual(sdm.N() , len(self.array[0]))
        testing.assert_array_equal(sdm, self.array)

    def testConstructor08(self):
        "Test Epetra.SerialDenseMatrix (2D-list,bool) constructor"
        sdm = Epetra.SerialDenseMatrix(self.array,False)
        self.assertEqual(sdm.CV(), Epetra.View       )
        self.assertEqual(sdm.M() , len(self.array)   )
        self.assertEqual(sdm.N() , len(self.array[0]))
        testing.assert_array_equal(sdm, self.array)

    def testConstructor09(self):
        "Test Epetra.SerialDenseMatrix (3D-list) constructor"
        list = [self.array, self.array]
        self.assertRaises(TypeError, Epetra.SerialDenseMatrix, list)

    def testConstructor10(self):
        "Test Epetra.SerialDenseMatrix (bad-list) constructor"
        myArray = [ [0, 1.2], ["e","pi"] ]
        self.assertRaises(TypeError, Epetra.SerialDenseMatrix, myArray)

    def testConstructor11(self):
        "Test Epetra.SerialDenseMatrix copy constructor for default"
        sdm1 = Epetra.SerialDenseMatrix()
        sdm2 = Epetra.SerialDenseMatrix(sdm1)
        self.assertEqual(sdm2.CV(), sdm1.CV())
        self.assertEqual(sdm1.M() , sdm2.M() )
        self.assertEqual(sdm1.N() , sdm2.N() )

    def testConstructor12(self):
        "Test Epetra.SerialDenseMatrix copy constructor for (int,int)"
        sdm1 = Epetra.SerialDenseMatrix(self.rows,self.cols)
        sdm2 = Epetra.SerialDenseMatrix(sdm1)
        self.assertEqual(sdm2.CV(), sdm1.CV())
        self.assertEqual(sdm1.M() , sdm2.M() )
        self.assertEqual(sdm1.N() , sdm2.N() )

    def testConstructor13(self):
        "Test Epetra.SerialDenseMatrix copy constructor for (2D-list)"
        sdm1 = Epetra.SerialDenseMatrix(self.array)
        sdm2 = Epetra.SerialDenseMatrix(sdm1)
        self.assertEqual(sdm2.CV(), sdm1.CV())
        self.assertEqual(sdm1.M() , sdm2.M() )
        self.assertEqual(sdm1.N() , sdm2.N() )

    def testShape1(self):
        "Test Epetra.SerialDenseMatrix Shape method"
        sdm = Epetra.SerialDenseMatrix()
        self.assertEqual(sdm.M(), 0)
        self.assertEqual(sdm.N(), 0)
        sdm.Shape(self.rows,self.cols)
        self.assertEqual(sdm.M(), self.rows)
        self.assertEqual(sdm.N(), self.cols)

    def testShape2(self):
        "Test Epetra.SerialDenseMatrix Shape method, negative rows"
        sdm = Epetra.SerialDenseMatrix()
        self.assertRaises(ValueError, sdm.Shape, -self.rows, self.cols)

    def testShape3(self):
        "Test Epetra.SerialDenseMatrix Shape method, negative columns"
        sdm = Epetra.SerialDenseMatrix()
        self.assertRaises(ValueError, sdm.Shape, self.rows, -self.cols)

    def testReshape1(self):
        "Test Epetra.SerialDenseMatrix Reshape method to smaller"
        sdm = Epetra.SerialDenseMatrix(self.array)
        self.assertEqual(sdm.M(), len(self.array)   )
        self.assertEqual(sdm.N(), len(self.array[0]))
        self.failUnless((sdm == self.array).all())
        result = sdm.Reshape(sdm.M()-1,sdm.N()-1)
        self.assertEqual(result, 0)
        self.assertEqual(sdm.M(), len(self.array)   -1)
        self.assertEqual(sdm.N(), len(self.array[0])-1)
        for i in range(sdm.M()):
            for j in range(sdm.N()):
                self.assertEqual(sdm[i,j],self.array[i][j])

    def testReshape2(self):
        "Test Epetra.SerialDenseMatrix Reshape method to larger"
        sdm = Epetra.SerialDenseMatrix(self.array)
        self.assertEqual(sdm.M(), len(self.array)   )
        self.assertEqual(sdm.N(), len(self.array[0]))
        for i in range(sdm.M()):
            for j in range(sdm.N()):
                self.assertEqual(sdm[i,j],self.array[i][j])
        result = sdm.Reshape(sdm.M()+1,sdm.N()+1)
        self.assertEqual(result, 0)
        self.assertEqual(sdm.M(), len(self.array)   +1)
        self.assertEqual(sdm.N(), len(self.array[0])+1)
        for i in range(sdm.M()):
            for j in range(sdm.N()):
                if (i < len(self.array)) and (j < len(self.array[0])):
                    self.assertEqual(sdm[i,j],self.array[i][j])
                else:
                    self.assertEqual(sdm[i,j],0.0)

    def testReshape3(self):
        "Test Epetra.SerialDenseMatrix Reshape method, negative rows"
        sdm = Epetra.SerialDenseMatrix()
        self.assertRaises(ValueError, sdm.Reshape, -self.rows, self.cols)

    def testReshape4(self):
        "Test Epetra.SerialDenseMatrix Reshape method, negative columns"
        sdm = Epetra.SerialDenseMatrix()
        self.assertRaises(ValueError, sdm.Reshape, self.rows, -self.cols)

    def testScale(self):
        "Test Epetra.SerialDenseMatrix Scale Method"
        sdm = Epetra.SerialDenseMatrix(self.array)
        self.failUnless((sdm == self.array).all())
        result = sdm.Scale(2)
        for i in range(sdm.M()):
            for j in range(sdm.N()):
                self.assertEqual(sdm[i,j],2*self.array[i][j])

    def testNormOne(self):
        "Test Epetra.SerialDenseMatrix NormOne method"
        a   = array(self.array)
        sdm = Epetra.SerialDenseMatrix(self.array)
        self.assertAlmostEqual(sdm.NormOne(), max(sum(abs(a),axis=0)))

    def testNormInf(self):
        "Test Epetra.SerialDenseMatrix NormInf method"
        a   = array(self.array)
        sdm = Epetra.SerialDenseMatrix(self.array)
        self.assertAlmostEqual(sdm.NormInf(), max(sum(abs(a),axis=1)))

    def testParentheses(self):
        "Test Epetra.SerialDenseMatrix __call__ method"
        sdm = Epetra.SerialDenseMatrix(self.array)
        for i in range(sdm.M()):
            for j in range(sdm.N()):
                self.assertEqual(sdm(i,j), self.array[i][j])

    def testGetItem1(self):
        "Test Epetra.SerialDenseMatrix __getitem__ method for two ints"
        sdm = Epetra.SerialDenseMatrix(self.array)
        for i in range(sdm.M()):
            for j in range(sdm.N()):
                self.assertEqual(sdm[i,j], self.array[i][j])

    def testGetItem2(self):
        "Test Epetra.SerialDenseMatrix __getitem__ method for int and slice"
        sdm = Epetra.SerialDenseMatrix(self.array)
        for i in range(sdm.M()):
            testing.assert_array_equal(sdm[i,:], self.array[i])

    def testGetItem3(self):
        "Test Epetra.SerialDenseMatrix __getitem__ method for two slices"
        sdm = Epetra.SerialDenseMatrix(self.array)
        testing.assert_array_equal(sdm[:,:], self.array)

    def testGetItem4(self):
        "Test Epetra.SerialDenseMatrix __getitem__ method for three ints"
        sdm = Epetra.SerialDenseMatrix(self.array)
        self.assertRaises(IndexError, sdm.__getitem__, (0,1,2))

    def testGetItem5(self):
        "Test Epetra.SerialDenseMatrix __getitem__ method for row out of range"
        sdm = Epetra.SerialDenseMatrix(self.array)
        self.assertRaises(IndexError, sdm.__getitem__, (self.rows,0))

    def testGetItem6(self):
        "Test Epetra.SerialDenseMatrix __getitem__ method for column out of range"
        sdm = Epetra.SerialDenseMatrix(self.array)
        self.assertRaises(IndexError, sdm.__getitem__, (0,self.cols))

    def testSetItem1(self):
        "Test Epetra.SerialDenseMatrix __setitem__ method for two ints"
        sdm = Epetra.SerialDenseMatrix(self.rows,self.cols)
        testing.assert_array_equal(sdm, zeros((self.rows,self.cols),'d'))
        for i in range(sdm.M()):
            for j in range(sdm.N()):
                value = i * sdm.N() + j
                sdm[i,j] = value
                self.assertEqual(sdm[i,j], value)

    def testSetItem2(self):
        "Test Epetra.SerialDenseMatrix __setitem__ method for int and slice"
        sdm = Epetra.SerialDenseMatrix(self.rows,self.cols)
        testing.assert_array_equal(sdm, zeros((self.rows,self.cols),'d'))
        for i in range(sdm.M()):
            sdm[i,:] = self.array[i]
            testing.assert_array_equal(sdm[i,:], self.array[i])

    def testSetItem3(self):
        "Test Epetra.SerialDenseMatrix __setitem__ method for two slices"
        sdm = Epetra.SerialDenseMatrix(self.rows,self.cols)
        testing.assert_array_equal(sdm, zeros((self.rows,self.cols),'d'))
        sdm[:,:] = self.array
        testing.assert_array_equal(sdm, self.array)

    def testSetItem4(self):
        "Test Epetra.SerialDenseMatrix __setitem__ method for three ints"
        sdm = Epetra.SerialDenseMatrix(self.array)
        self.assertRaises(IndexError, sdm.__setitem__, (0,1,2), 3.14)

    def testSetItem5(self):
        "Test Epetra.SerialDenseMatrix __setitem__ method for row out of range"
        sdm = Epetra.SerialDenseMatrix(self.array)
        self.assertRaises(IndexError, sdm.__setitem__, (self.rows,0), 3.14)

    def testSetItem6(self):
        "Test Epetra.SerialDenseMatrix __setitem__ method for column out of range"
        sdm = Epetra.SerialDenseMatrix(self.array)
        self.assertRaises(IndexError, sdm.__setitem__, (0,self.cols), 3.14)

    def testPrint(self):
        "Test Epetra.SerialDenseMatrix Print method"
        n   = self.size
        sdm = Epetra.SerialDenseMatrix(n,n)
        sdm[:,:] = 0.0
        filename = "testSerialDense%d.dat" % self.comm.MyPID()
        f = open(filename, "w")
        sdm.Print(f)
        f.close()
        out = "\nData access mode: %s\nA_Copied: %s\nRows(M): %d\nColumns(N): %d\nLDA: %d\n" \
              % (self.CVStr,self.YNStr,n,n,n) + (n * "0 " + "\n") * n
        f = open(filename, "r")
        self.assertEqual(f.read(), out)
        f.close()

    def testStr(self):
        "Test Epetra.SerialDenseMatrix __str__ method"
        n   = self.size
        sdv = Epetra.SerialDenseMatrix(n,n)
        sdv[:,:] = 0.0
        row = "[" + n * " 0. "
        row = row[:-1] + "]\n "
        out = "[" + n*row
        out = out[:-2] + "]"
        self.assertEqual(str(sdv), out)

    def testInPlaceAdd(self):
        "Test Epetra.SerialDenseMatrix += operator"
        sdm = Epetra.SerialDenseMatrix(self.array)
        self.failUnless((sdm == self.array).all())
        sdm += sdm
        for i in range(sdm.M()):
            for j in range(sdm.N()):
                self.assertEqual(sdm[i,j],2*self.array[i][j])

    def testRandom(self):
        "Test Epetra.SerialDenseMatrix Random method"
        sdm = Epetra.SerialDenseMatrix(self.size,self.size)
        sdm[:,:] = 0.0
        self.failUnless((sdm == 0.0).all())
        result = sdm.Random()
        self.assertEqual(result, 0)
        for i in range(self.size):
            for j in range(self.size):
                self.failUnless(sdm[i,j] > -1.0)
                self.failUnless(sdm[i,j] <  1.0)

    def testM(self):
        "Test Epetra.SerialDenseMatrix M method"
        for numRows in range(4,10):
            for numCols in range(4,10):
                sdm = Epetra.SerialDenseMatrix(numRows,numCols)
                self.assertEqual(sdm.M(), numRows)

    def testN(self):
        "Test Epetra.SerialDenseMatrix N method"
        for numRows in range(4,10):
            for numCols in range(4,10):
                sdm = Epetra.SerialDenseMatrix(numRows,numCols)
                self.assertEqual(sdm.N(), numCols)

    def testA1(self):
        "Test Epetra.SerialDenseMatrix A method for default"
        sdm = Epetra.SerialDenseMatrix()
        testing.assert_array_equal(sdm.A(), sdm.array)

    def testA2(self):
        "Test Epetra.SerialDenseMatrix A method for (int,int)"
        sdm = Epetra.SerialDenseMatrix(self.rows,self.cols)
        result = sdm.Random()
        testing.assert_array_equal(sdm.A(), sdm.array)

    def testA3(self):
        "Test Epetra.SerialDenseMatrix A method for (2D-array)"
        sdm = Epetra.SerialDenseMatrix(self.array)
        testing.assert_array_equal(sdm.A(), sdm.array)

    def testArray(self):
        "Test Epetra.SerialDenseMatrix array attribute"
        sdm = Epetra.SerialDenseMatrix()
        self.assertRaises(AttributeError, sdm.__setattr__, 'array', None)

    def testLDA(self):
        "Test Epetra.SerialDenseMatrix LDA method"
        for numRows in range(4,10):
            for numCols in range(4,10):
                sdm = Epetra.SerialDenseMatrix(numRows,numCols)
                self.assertEqual(sdm.LDA(),sdm.M())

    def testUseTranspose(self):
        "Test Epetra.SerialDenseMatrix UseTranspose method"
        sdm = Epetra.SerialDenseMatrix(self.array)
        self.assertEqual(sdm.UseTranspose(), False)

    def testSetUseTranspose(self):
        "Test Epetra.SerialDenseMatrix SetUseTranspose method"
        sdm = Epetra.SerialDenseMatrix(self.array)
        for state in (True,False):
            sdm.SetUseTranspose(state)
            self.assertEqual(sdm.UseTranspose(), state)

    def testLabel(self):
        "Test Epetra.SerialDenseMatrix Label method"
        sdm = Epetra.SerialDenseMatrix(self.rows,self.cols)
        self.assertEqual(sdm.Label(), "Epetra::SerialDenseMatrix")

    def testHasNormInf(self):
        "Test Epetra.SerialDenseMatrix HasNormInf method"
        sdm = Epetra.SerialDenseMatrix(self.array)
        self.assertEqual(sdm.HasNormInf(), True)

    def testRowDim(self):
        "Test Epetra.SerialDenseMatrix RowDim method"
        sdm = Epetra.SerialDenseMatrix(self.rows,self.cols)
        self.assertEqual(sdm.RowDim(), sdm.M())

    def testColDim(self):
        "Test Epetra.SerialDenseMatrix ColDim method"
        sdm = Epetra.SerialDenseMatrix(self.array)
        self.assertEqual(sdm.ColDim(), sdm.N())

    def testMultiply1(self):
        "Test Epetra.SerialDenseMatrix Multiply method with trivial addition"
        this = Epetra.SerialDenseMatrix(3,4)
        a    = Epetra.SerialDenseMatrix([[ 1, 16],
                                         [ 7, 10],
                                         [13,  4]])
        b    = Epetra.SerialDenseMatrix([[0, 2, 4,  6],
                                         [5, 3, 1, -1]])
        c    = Epetra.SerialDenseMatrix([[80, 50, 20, -10],
                                         [50, 44, 38,  32],
                                         [20, 38, 56,  74]])
        result = this.Multiply("N","N",1.0,a,b,1.0)
        self.assertEqual(result, 0)
        testing.assert_array_equal(this, c)

    def testMultiply2(self):
        "Test Epetra.SerialDenseMatrix Multiply method with addition"
        a = Epetra.SerialDenseMatrix([[ 1, 16],
                                      [ 7, 10],
                                      [13,  4]])
        b = Epetra.SerialDenseMatrix([[0, 2, 4,  6],
                                      [5, 3, 1, -1]])
        c = Epetra.SerialDenseMatrix([[80, 50, 20, -10],
                                      [50, 44, 38,  32],
                                      [20, 38, 56,  74]])
        d = Epetra.SerialDenseMatrix(c) * 2
        result = c.Multiply("N","N",1.0,a,b,1.0)
        self.assertEqual(result, 0)
        testing.assert_array_equal(c, d)

    def testMultiply3(self):
        "Test Epetra.SerialDenseMatrix Multiply method with no addition"
        a   = Epetra.SerialDenseMatrix([[ 1, 16],
                                        [ 7, 10],
                                        [13,  4]])
        x   = Epetra.SerialDenseMatrix([[0, 2, 4,  6],
                                        [5, 3, 1, -1]])
        y   = Epetra.SerialDenseMatrix(3,4)
        ans = Epetra.SerialDenseMatrix([[80, 50, 20, -10],
                                      [50, 44, 38,  32],
                                      [20, 38, 56,  74]])
        result = a.Multiply(False,x,y)
        self.assertEqual(result, 0)
        testing.assert_array_equal(y, ans)

    def testApply(self):
        "Test Epetra.SerialDenseMatrix Apply method"
        a   = Epetra.SerialDenseMatrix([[ 1, 16],
                                        [ 7, 10],
                                        [13,  4]])
        x   = Epetra.SerialDenseMatrix([[0, 2, 4,  6],
                                        [5, 3, 1, -1]])
        y   = Epetra.SerialDenseMatrix(3,4)
        ans = Epetra.SerialDenseMatrix([[80, 50, 20, -10],
                                        [50, 44, 38,  32],
                                        [20, 38, 56,  74]])
        result = a.Apply(x,y)
        self.assertEqual(result, 0)
        testing.assert_array_equal(y, ans)

    def testApplyInverse(self):
        "Test Epetra.SerialDenseMatrix ApplyInverse method"
        a   = Epetra.SerialDenseMatrix([[ 1, 16],
                                        [ 7, 10]])
        x   = Epetra.SerialDenseMatrix([[0, 2],
                                        [5, 3]])
        y   = Epetra.SerialDenseMatrix(2,2)
        result = a.ApplyInverse(x,y)
        self.assertEqual(result, -1)

##########################################################################

class EpetraSerialDenseVectorTestCase(unittest.TestCase):
    "TestCase class for Epetra SerialDense Vectors"

    def setUp(self):
        self.comm = Epetra.PyComm()
        self.size = 4
        self.rows = 2
        self.cols = 4
        self.list = [0.0, -1, 2.17, -3.14]
        # Query whether Copy objects are actually Copy objects.  For older
        # versions of SWIG, they are View objects, but newer versions of SWIG
        # get it right
        sdv = Epetra.SerialDenseVector(self.size)
        self.CV = sdv.CV()
        if self.CV == Epetra.View:
            self.CVStr = "View"
            self.YNStr = "no"
        else:
            self.CVStr = "Copy"
            self.YNStr = "yes"

    def tearDown(self):
        self.comm.Barrier()

    def testConstructor0(self):
        "Test Epetra.SerialDenseVector default constructor"
        sdv = Epetra.SerialDenseVector()
        self.assertEqual(sdv.CV(), Epetra.Copy)
        self.assertEqual(sdv.Length(), 0)

    def testConstructor1(self):
        "Test Epetra.SerialDenseVector sized constructor"
        sdv = Epetra.SerialDenseVector(self.size)
        self.assertEqual(sdv.CV(), self.CV)
        self.assertEqual(sdv.Length(), self.size)

    def testConstructor2(self):
        "Test Epetra.SerialDenseVector 1D list constructor"
        sdv = Epetra.SerialDenseVector(self.list)
        self.assertEqual(sdv.CV(), Epetra.View)
        self.assertEqual(sdv.Length(), len(self.list))
        self.failUnless((sdv == self.list).all())

    def testConstructor3(self):
        "Test Epetra.SerialDenseVector 2D list constructor"
        list = [[0.0, -1, 2.17], [-3.14, 4, -5.5]]
        sdv = Epetra.SerialDenseVector(list)
        self.assertEqual(sdv.CV(), Epetra.View)
        self.assertEqual(sdv.Length(), len(list)*len(list[0]))
        self.failUnless((sdv == list).all())

    def testConstructor4(self):
        "Test Epetra.SerialDenseVector copy constructor"
        sdv1 = Epetra.SerialDenseVector(self.size)
        sdv2 = Epetra.SerialDenseVector(sdv1)
        self.assertEqual(sdv2.CV(), self.CV)
        self.assertEqual(sdv1.Length(), sdv2.Length())

    def testConstructor5(self):
        "Test Epetra.SerialDenseVector bad-list constructor"
        list = [0,1.0,"e","pi"]
        self.assertRaises(TypeError,Epetra.SerialDenseVector,list)

    def testPrint(self):
        "Test Epetra.SerialDenseVector Print method"
        sdv = Epetra.SerialDenseVector(self.size)
        sdv[:] = 0.0
        filename = "testSerialDense%d.dat" % self.comm.MyPID()
        f = open(filename, "w")
        sdv.Print(f)
        f.close()
        out = "Data access mode: %s\nA_Copied: %s\nLength(M): %d\n" % \
              (self.CVStr,self.YNStr,self.size) + self.size * "0 " + "\n"
        f = open(filename, "r")
        self.assertEqual(f.read(), out)
        f.close()

    def testStr(self):
        "Test Epetra.SerialDenseVector __str__ method"
        sdv = Epetra.SerialDenseVector(self.size)
        sdv[:] = 0.0
        out = "[" + self.size * " 0. "
        out = out [:-1] + "]"
        self.assertEquals(str(sdv), out)

    def testSize(self):
        "Test Epetra.SerialDenseVector Size method"
        sdv = Epetra.SerialDenseVector()
        result = sdv.Size(3*self.size)
        self.assertEqual(result, 0)
        self.assertEqual(sdv.Length(), 3*self.size)
        self.assertEqual(sdv.Length(), len(sdv))
        self.failUnless((sdv == 0.0).all())

    def testResize1(self):
        "Test Epetra.SerialDenseVector Resize method to smaller"
        sdv = Epetra.SerialDenseVector(3*self.size)
        self.assertEqual(sdv.Length(), 3*self.size)
        self.assertEqual(sdv.Length(), len(sdv))
        sdv[:] = range(len(sdv))
        result = sdv.Resize(self.size)
        self.assertEqual(result, 0)
        self.assertEqual(sdv.Length(), self.size)
        self.assertEqual(sdv.Length(), len(sdv))
        for i in range(len(sdv)):
            self.assertEqual(sdv[i], i)

    def testResize2(self):
        "Test Epetra.SerialDenseVector Resize method to larger"
        sdv = Epetra.SerialDenseVector(self.size)
        self.assertEqual(sdv.Length(), self.size)
        self.assertEqual(sdv.Length(), len(sdv))
        sdv[:] = range(len(sdv))
        sdv.Resize(3*self.size)
        self.assertEqual(sdv.Length(), 3*self.size)
        self.assertEqual(sdv.Length(), len(sdv))
        for i in range(len(sdv)):
            if i < self.size:
                self.assertEqual(sdv[i], i)
            else:
                self.assertEqual(sdv[i], 0.0)

    def testGetItem1(self):
        "Test Epetra.SerialDenseVector __getitem__ method"
        sdv = Epetra.SerialDenseVector(self.list)
        for i in range(len(self.list)):
            self.assertEqual(sdv[i],self.list[i])

    def testGetItem2(self):
        "Test Epetra.SerialDenseVector __call__ method"
        sdv = Epetra.SerialDenseVector(self.list)
        for i in range(len(self.list)):
            self.assertEqual(sdv(i),self.list[i])

    def testSetItem(self):
        "Test Epetra.SerialDenseVector __setitem__ method"
        sdv = Epetra.SerialDenseVector(len(self.list))
        sdv[:] = 0.0
        self.failUnless((sdv == 0.0).all())
        for i in range(len(sdv)):
            sdv[i] = self.list[i]
        self.failUnless((sdv == self.list).all())

    def testIndexErrors(self):
        "Test Epetra.SerialDenseVector index errors"
        sdv = Epetra.SerialDenseVector(self.size)
        self.assertRaises(TypeError, sdv.__getitem__, 0,1)
        self.assertRaises(TypeError, sdv.__setitem__, 0,1,3.14)

    def testRandom(self):
        "Test Epetra.SerialDenseVector Random method"
        sdv = Epetra.SerialDenseVector(self.size)
        sdv[:] = 0.0
        self.failUnless((sdv == 0.0).all())
        result = sdv.Random()
        self.assertEqual(result, 0)
        for i in range(self.size):
            self.failUnless(sdv[i] > -1.0)
            self.failUnless(sdv[i] <  1.0)

    def testDot1(self):
        "Test Epetra.SerialDenseVector Dot method with self"
        a   = array(self.list)
        sdv = Epetra.SerialDenseVector(self.list)
        self.assertEqual(sdv.Dot(sdv), sum(a*a))

    def testDot2(self):
        "Test Epetra.SerialDenseVector Dot method with other"
        a    = array(self.list)
        b    = arange(len(self.list))
        sdv1 = Epetra.SerialDenseVector(self.list)
        sdv2 = Epetra.SerialDenseVector(b)
        self.assertEqual(sdv1.Dot(sdv2), sum(a*b))

    def testNorm1(self):
        "Test Epetra.SerialDenseVector Norm1 method"
        a   = array(self.list)
        sdv = Epetra.SerialDenseVector(self.list)
        self.assertEqual(sdv.Norm1(), sum(abs(a)))

    def testNorm2(self):
        "Test Epetra.SerialDenseVector Norm2 method"
        a   = array(self.list)
        sdv = Epetra.SerialDenseVector(self.list)
        self.assertEqual(sdv.Norm2(), sqrt(sum(a*a)))

    def testNormInf(self):
        "Test Epetra.SerialDenseVector NormInf method"
        a   = array(self.list)
        sdv = Epetra.SerialDenseVector(self.list)
        self.assertEqual(sdv.NormInf(), max(abs(a)))

    def testLength(self):
        "Test Epetra.SerialDenseVector Length method"
        sdv = Epetra.SerialDenseVector()
        self.assertEqual(sdv.Length(),0)
        self.assertEqual(sdv.Length(),len(sdv))
        sdv.Size(self.size)
        self.assertEqual(sdv.Length(),self.size)
        self.assertEqual(sdv.Length(),len(sdv))
        sdv.Resize(2*self.size)
        self.assertEqual(sdv.Length(),2*self.size)
        self.assertEqual(sdv.Length(),len(sdv))
        sdv.Resize(self.size)
        self.assertEqual(sdv.Length(),self.size)
        self.assertEqual(sdv.Length(),len(sdv))

    def testValues(self):
        "Test Epetra.SerialDenseVector Values method"
        sdv  = Epetra.SerialDenseVector(self.list)
        vals = sdv.Values()
        self.assertNotEqual(type(sdv), type(vals))
        self.assertEqual(sdv.Length(), len(vals))
        for i in range(len(vals)):
            self.assertEqual(sdv[i], vals[i])

    def testScale(self):
        "Test Epetra.SerialDenseVector Scale Method"
        sdv = Epetra.SerialDenseVector(self.list)
        self.failUnless((sdv == self.list).all())
        result = sdv.Scale(2)
        for i in range(sdv.Length()):
            self.assertEqual(sdv[i],2*self.list[i])

    def testInPlaceAdd(self):
        "Test Epetra.SerialDenseVector += operator"
        sdv = Epetra.SerialDenseVector(self.list)
        self.failUnless((sdv == self.list).all())
        sdv += sdv
        for i in range(sdv.Length()):
            self.assertEqual(sdv[i],2*self.list[i])

##########################################################################

class EpetraSerialDenseSolverTestCase(unittest.TestCase):
    "TestCase class for Epetra SerialDense Solvers"

    def setUp(self):
        self.comm = Epetra.PyComm()
        self.size = 9
        self.A    = Epetra.SerialDenseMatrix(self.size,self.size)
        self.x    = Epetra.SerialDenseVector(self.size)
        self.b    = Epetra.SerialDenseVector(self.size)
        for i in range(1,self.size-1):
            self.A[i,i-1:i+2] = [-1,2,-1]
        self.A[ 0, 0] = 1
        self.A[-1,-1] = 1
        self.b[-1]    = 1
        self.soln     = arange(self.size) / (self.size-1.0)

    def tearDown(self):
        self.comm.Barrier()

    def buildProblem(self,sds):
        result = sds.SetMatrix(self.A)
        self.assertEqual(result,0)
        result = sds.SetVectors(self.x,self.b)
        self.assertEqual(result,0)

    def solveProblem(self,sds):
        result = sds.Factor()
        self.assertEqual(result,0)
        result = sds.Solve()
        self.assertEqual(result,0)

    def testConstructor(self):
        "Test Epetra.SerialDenseSolver constructor"
        sds = Epetra.SerialDenseSolver()
        self.assertEqual(sds.Transpose()                   ,False)
        self.assertEqual(sds.Factored()                    ,False)
        self.assertEqual(sds.A_Equilibrated()              ,False)
        self.assertEqual(sds.B_Equilibrated()              ,False)
        self.assertEqual(sds.SolutionErrorsEstimated()     ,False)
        self.assertEqual(sds.Inverted()                    ,False)
        self.assertEqual(sds.ReciprocalConditionEstimated(),False)
        self.assertEqual(sds.Solved()                      ,False)
        self.assertEqual(sds.SolutionRefined()             ,False)

    def testSetMatrix(self):
        "Test Epetra.SerialDenseSolver SetMatrix method"
        sds = Epetra.SerialDenseSolver()
        result = sds.SetMatrix(self.A)
        self.assertEqual(result, 0)
        self.failUnless(isinstance(sds.Matrix(),Epetra.SerialDenseMatrix))
        testing.assert_array_equal(self.A, sds.Matrix())

    def testSetVectors(self):
        "Test Epetra.SerialDenseSolver SetVectors method"
        sds = Epetra.SerialDenseSolver()
        result = sds.SetVectors(self.x,self.b)
        self.assertEqual(result, 0)
        self.failUnless(isinstance(sds.LHS(),Epetra.SerialDenseMatrix))
        self.failUnless(isinstance(sds.RHS(),Epetra.SerialDenseMatrix))
        testing.assert_array_equal(self.x, sds.LHS()[:,0])
        testing.assert_array_equal(self.b, sds.RHS()[:,0])

    def testMisMatch(self):
        "Test Epetra.SerialDenseSolver with mismatched dimensions"
        sds = Epetra.SerialDenseSolver()
        self.x.Resize(2*self.size)
        result = sds.SetMatrix(self.A)
        self.assertEqual(result, 0)
        result = sds.SetVectors(self.x,self.b)
        self.assertEqual(result, -1)

    def testEquilibration(self):
        "Test Epetra.SerialDenseSolver FactorWithEquilibration method"
        sds = Epetra.SerialDenseSolver()
        self.assertEqual(sds.A_Equilibrated(), False)
        self.assertEqual(sds.B_Equilibrated(), False)
        self.buildProblem(sds)
        sds.FactorWithEquilibration(True)
        self.solveProblem(sds)
        self.assertEqual(sds.A_Equilibrated(), True )
        self.assertEqual(sds.B_Equilibrated(), True)

    def testTranspose(self):
        "Test Epetra.SerialDenseSolver SolveWithTranspose and Transpose methods"
        sds = Epetra.SerialDenseSolver()
        self.assertEqual(sds.Transpose(), False)
        sds.SolveWithTranspose(True)
        self.assertEqual(sds.Transpose(), True)

    def testSolveToRefinedSolution(self):
        "Test Epetra.SerialDenseSolver SolveToRefinedSolution and SolutionRefined methods"
        sds = Epetra.SerialDenseSolver()
        self.assertEqual(sds.SolutionRefined(), False)
        sds.SolveToRefinedSolution(True)
        self.buildProblem(sds)
        self.solveProblem(sds)
        self.assertEqual(sds.SolutionRefined(), True)

    def testEstimateSolutionErrors(self):
        "Test Epetra.SerialDenseSolver EstimateSolutionErrors method"
        sds = Epetra.SerialDenseSolver()
        self.assertEqual(sds.SolutionErrorsEstimated(), False)
        self.buildProblem(sds)
        sds.EstimateSolutionErrors(True)
        self.solveProblem(sds)
        self.assertEqual(sds.SolutionErrorsEstimated(), True)

    def testFactor(self):
        "Test Epetra.SerialDenseSolver Factor and Factored methods"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        self.assertEqual(sds.Factored(), False)
        result = sds.Factor()
        self.assertEqual(result,0)
        self.assertEqual(sds.Factored(), True)

    def testSolve(self):
        "Test Epetra.SerialDenseSolver Solve and Solved methods"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        self.assertEqual(sds.Solved(), False)
        self.solveProblem(sds)
        self.assertEqual(sds.Solved(), True)

    def testInvert(self):
        "Test Epetra.SerialDenseSolver Invert and Inverted methods"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        self.assertEqual(sds.Inverted(), False)
        result = sds.Invert()
        self.assertEqual(result,0)
        self.assertEqual(sds.Inverted(), True)

    def testEquilibrateScaling(self):
        "Test Epetra.SerialDenseSolver ComputeEquilibrateScaling method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        self.assertEqual(sds.C(), None)
        self.assertEqual(sds.R(), None)
        result = sds.ComputeEquilibrateScaling()
        self.assertEqual(result, 0)
        testing.assert_array_equal(sds.C(), 1.0)
        testing.assert_array_equal(sds.R(), 1/diagonal(self.A))

    def testEquilibrateMatrix(self):
        "Test Epetra.SerialDenseSolver EquilibrateMatrix method"
        sds = Epetra.SerialDenseSolver()
        A0  = mat(self.A,copy=True)
        self.buildProblem(sds)
        result = sds.EquilibrateMatrix()
        self.assertEqual(result,0)
        self.assertEqual(sds.A_Equilibrated(), True)
        S = mat(identity(self.size,'d') / diagonal(A0))
        testing.assert_array_equal(sds.Matrix(), S*A0)

    def testEquilibrateRHS(self):
        "Test Epetra.SerialDenseSolver EquilibrateRHS method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        result = sds.EquilibrateRHS()
        self.assertEqual(result,0)
        self.assertEqual(sds.B_Equilibrated(), True)

    def testApplyRefinement(self):
        "Test Epetra.SerialDenseSolver ApplyRefinement method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        sds.SolveToRefinedSolution(True)
        # solveProblem() wll call Solve() and Solve() will call ApplyRefinement()
        self.solveProblem(sds)
        self.assertEqual(sds.SolutionRefined(), True)

    def testUnequilibrateLHS(self):
        "Test Epetra.SerialDenseSolver Unequilibrate method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        sds.FactorWithEquilibration(True)
        self.solveProblem(sds)
        testing.assert_array_almost_equal(sds.LHS()[:,0], self.soln)
        result = sds.UnequilibrateLHS()
        self.assertEqual(result,0)
        testing.assert_array_almost_equal(sds.LHS()[:,0], self.soln)

    def testReciprocalConditionEstimate(self):
        "Test Epetra.SerialDenseSolver ReciprocalConditionEstimate method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        rcond = sds.ReciprocalConditionEstimate()
        self.assertAlmostEqual(rcond, 1.0/32)

    # This test will fail with a bus error until Bug 2224 is resolved

    #def testReciprocalConditionEstimateBad(self):
    #    "Test Epetra.SerialDenseSolver ReciprocalConditionEstimate method, improper use"
    #    sds = Epetra.SerialDenseSolver()
    #    self.assertRaises(RuntimeError, sds.ReciprocalConditionEstimate)

    def testShouldEquilibrate(self):
        "Test Epetra.SerialDenseSolver ShouldEquilibrate method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        self.assertEqual(sds.ShouldEquilibrate(), False)

    def testSolutionErrorsEstimated(self):
        "Test Epetra.SerialDenseSolver SolutionErrorsEstimated method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        sds.EstimateSolutionErrors(True)
        self.assertEqual(sds.SolutionErrorsEstimated(), False)
        self.solveProblem(sds)
        self.assertEqual(sds.SolutionErrorsEstimated(), True)

    def testReciprocalConditionEstimated(self):
        "Test Epetra.SerialDenseSolver ReciprocalConditionEstimated method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        self.assertEqual(sds.ReciprocalConditionEstimated(), False)
        rc = sds.ReciprocalConditionEstimate()
        self.assertEqual(sds.ReciprocalConditionEstimated(), True)

    def testMatrix(self):
        "Test Epetra.SerialDenseSolver Matrix method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        matrix = sds.Matrix()
        self.assertEqual(isinstance(matrix,Epetra.SerialDenseMatrix), True)
        testing.assert_array_equal(matrix, self.A)

    def testMatrixBad(self):
        "Test Epetra.SerialDenseSolver Matrix method, called before problem defined"
        sds = Epetra.SerialDenseSolver()
        self.assertEqual(sds.Matrix(), None)

    def testFactoredMatrix(self):
        "Test Epetra.SerialDenseSolver FactoredMatrix method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        matrix = sds.FactoredMatrix()
        self.assertEqual(isinstance(matrix,Epetra.SerialDenseMatrix), True)
        testing.assert_array_equal(matrix, self.A)
        result = sds.Factor()
        self.assertEqual(result,0)
        matrix = sds.FactoredMatrix()
        self.assertEqual(isinstance(matrix,Epetra.SerialDenseMatrix), True)

    def testFactoredMatrixBad(self):
        "Test Epetra.SerialDenseSolver FactoredMatrix method, called before problem defined"
        sds = Epetra.SerialDenseSolver()
        self.assertEqual(sds.FactoredMatrix(), None)

    def testLHS(self):
        "Test Epetra.SerialDenseSolver LHS method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        vectors = sds.LHS()
        self.assertEqual(isinstance(vectors,Epetra.SerialDenseMatrix), True)
        testing.assert_array_equal(vectors[:,0], self.x)

    def testLHSBad(self):
        "Test Epetra.SerialDenseSolver LHS method, called before problem defined"
        sds = Epetra.SerialDenseSolver()
        self.assertEqual(sds.LHS(), None)

    def testRHS(self):
        "Test Epetra.SerialDenseSolver RHS method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        vectors = sds.RHS()
        self.assertEqual(isinstance(vectors,Epetra.SerialDenseMatrix), True)
        testing.assert_array_equal(vectors[:,0], self.b)

    def testRHSBad(self):
        "Test Epetra.SerialDenseSolver RHS method, called before problem defined"
        sds = Epetra.SerialDenseSolver()
        self.assertEqual(sds.RHS(), None)

    def testM(self):
        "Test Epetra.SerialDenseSolver M method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        self.assertEqual(sds.M(), self.size)

    def testN(self):
        "Test Epetra.SerialDenseSolver N method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        self.assertEqual(sds.N(), self.size)

    def testA(self):
        "Test Epetra.SerialDenseSolver A method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        A = sds.A()
        self.failUnless(isinstance(A,ndarray))
        testing.assert_array_equal(A, self.A)

    def testABad(self):
        "Test Epetra.SerialDenseSolver A method, called before problem defined"
        sds = Epetra.SerialDenseSolver()
        self.assertEqual(sds.A(), None)

    def testLDA(self):
        "Test Epetra.SerialDenseSolver LDA method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        self.assertEqual(sds.LDA(), self.size)

    def testB(self):
        "Test Epetra.SerialDenseSolver B method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        B = sds.B()
        self.failUnless(isinstance(B,ndarray))
        testing.assert_array_equal(B[:,0], self.b)

    def testBBad(self):
        "Test Epetra.SerialDenseSolver B method, called before problem defined"
        sds = Epetra.SerialDenseSolver()
        self.assertEqual(sds.B(), None)

    def testLDB(self):
        "Test Epetra.SerialDenseSolver LDB method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        self.assertEqual(sds.LDB(), self.size)

    def testNRHS(self):
        "Test Epetra.SerialDenseSolver NRHS method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        self.assertEqual(sds.NRHS(), 1)

    def testX(self):
        "Test Epetra.SerialDenseSolver X method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        X = sds.X()
        self.failUnless(isinstance(X,ndarray))
        testing.assert_array_equal(X[:,0], self.x)

    def testXBad(self):
        "Test Epetra.SerialDenseSolver X method, called before problem defined"
        sds = Epetra.SerialDenseSolver()
        self.assertEqual(sds.X(), None)

    def testLDX(self):
        "Test Epetra.SerialDenseSolver LDX method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        self.assertEqual(sds.LDX(), self.size)

    def testAF(self):
        "Test Epetra.SerialDenseSolver AF method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        AF = sds.AF()
        self.failUnless(isinstance(AF,ndarray))
        testing.assert_array_equal(AF, self.A)

    def testAFBad(self):
        "Test Epetra.SerialDenseSolver AF method, called before problem defined"
        sds = Epetra.SerialDenseSolver()
        self.assertEqual(sds.AF(), None)

    def testLDAF(self):
        "Test Epetra.SerialDenseSolver LDAF method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        self.assertEqual(sds.LDAF(), self.size)

    def testIPIV(self):
        "Test Epetra.SerialDenseSolver IPIV method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        result = sds.Factor()
        self.assertEqual(result,0)
        testing.assert_array_equal(sds.IPIV(), arange(self.size)+1)

    def testIPIVBad(self):
        "Test Epetra.SerialDenseSolver IPIV method, called before problem defined"
        sds = Epetra.SerialDenseSolver()
        self.assertEqual(sds.IPIV(), None)

    def testANORM(self):
        "Test Epetra.SerialDenseSolver ANORM method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        result = sds.ANORM()
        self.assertEqual(result,-1.0)
        A0 = Epetra.SerialDenseMatrix(self.A)
        sds.Factor()
        result = sds.ANORM()
        self.assertEqual(result,A0.OneNorm())

    def testRCOND(self):
        "Test Epetra.SerialDenseSolver RCOND method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        self.assertEqual(sds.RCOND(), -1.0)
        rc = sds.ReciprocalConditionEstimate()
        self.assertEqual(sds.RCOND(), rc)

    def testROWCND(self):
        "Test Epetra.SerialDenseSolver ROWCND method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        result = sds.ROWCND()
        self.assertEqual(result,-1.0)
        sds.ComputeEquilibrateScaling()
        result = sds.ROWCND()
        self.assertEqual(result,0.5)

    def testCOLCND(self):
        "Test Epetra.SerialDenseSolver COLCND method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        result = sds.COLCND()
        self.assertEqual(result,-1.0)
        sds.ComputeEquilibrateScaling()
        result = sds.COLCND()
        self.assertEqual(result,1.0)

    def testAMAX(self):
        "Test Epetra.SerialDenseSolver AMAX method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        result = sds.AMAX()
        self.assertEqual(result,-1.0)
        sds.ComputeEquilibrateScaling()
        result = sds.AMAX()
        self.assertEqual(result,2.0)

    def testFERR(self):
        "Test Epetra.SerialDenseSolver FERR method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        sds.SolveToRefinedSolution(True)
        self.solveProblem(sds)
        self.failUnless(sds.FERR()[0] < 1.0e-12)

    def testFERRBad(self):
        "Test Epetra.SerialDenseSolver FERR method, called before problem defined"
        sds = Epetra.SerialDenseSolver()
        self.assertEqual(sds.FERR(), None)

    def testBERR(self):
        "Test Epetra.SerialDenseSolver BERR method"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        sds.SolveToRefinedSolution(True)
        self.solveProblem(sds)
        self.assertEqual(sds.BERR()[0], 1.0)  # Case when denominator == 0?

    def testBERRBad(self):
        "Test Epetra.SerialDenseSolver BERR method, called before problem defined"
        sds = Epetra.SerialDenseSolver()
        self.assertEqual(sds.BERR(), None)

    def testPrint(self):
        "Test Epetra.SerialDenseSolver Print method()"
        sds = Epetra.SerialDenseSolver()
        self.buildProblem(sds)
        filename = "testSerialDense%d.dat" % self.comm.MyPID()
        f = open(filename, "w")
        sds.Print(f)
        f.close()
        output = open(filename,"r").read()
        self.assertEqual(len(output), 815)

##########################################################################

if __name__ == "__main__":

    # Create the test suite object
    suite = unittest.TestSuite()

    # Add the test cases to the test suite
    suite.addTest(unittest.makeSuite(EpetraSerialDenseMatrixTestCase))
    suite.addTest(unittest.makeSuite(EpetraSerialDenseVectorTestCase))
    suite.addTest(unittest.makeSuite(EpetraSerialDenseSolverTestCase))

    # Create a communicator
    comm    = Epetra.PyComm()
    iAmRoot = comm.MyPID() == 0
    comm.SetTracebackMode(0)    # Turn off Epetra errors printed to stderr

    # Run the test suite
    if iAmRoot: print >>sys.stderr, \
       "\n**************************\nTesting Epetra.SerialDense\n**************************\n"
    verbosity = 2 * int(iAmRoot)
    result = unittest.TextTestRunner(verbosity=verbosity).run(suite)

    # Exit with a code that indicates the total number of errors and failures
    errsPlusFails = comm.SumAll(len(result.errors) + len(result.failures))
    if errsPlusFails == 0 and iAmRoot: print "End Result: TEST PASSED"
    sys.exit(errsPlusFails)
