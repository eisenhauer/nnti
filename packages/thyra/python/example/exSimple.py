#! /usr/bin/env python

# @HEADER
# ************************************************************************
# 
#               PyTrilinos.Thyra: Python Interface to Thyra
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

from optparse import *

parser = OptionParser()
parser.add_option("-t", "--testharness", action="store_true",
                  dest="testharness", default=False,
                  help="test local build modules; prevent loading system-installed modules")
parser.add_option("-v", "--verbosity", type="int", dest="verbosity", default=2,
                  help="set the verbosity level [default 2]")
options,args = parser.parse_args()
if options.testharness:
    import setpath
    import Epetra, Thyra
else:
    try:
        import setpath
        import Epetra, Thyra
    except ImportError:
        from PyTrilinos import Epetra, Thyra
        print >>sys.stderr, "Using system-installed Epetra, Thyra"

vs = Thyra.SerialVectorSpaceStd_double(4)
print "vs.dim() =", vs.dim()
print "dir(vs) =", dir(vs)
v = Thyra.createMember_double(vs)
print "type(v) =", type(v)
print "v.this =", v.this
print "dir(v) =", dir(v)
print "v.space().dim() =", v.space().dim()
v2 = Thyra.createMember_double(v.space())
print "v2.space().dim() =", v2.space().dim()
Thyra.V_S_double(v.__deref__(),1.0)
#Thyra.V_S_double(v,1.0)
print "sum_double(v) =", Thyra.sum_double(v.__deref__())
#print "sum_double(v) =", Thyra.sum_double(v)
