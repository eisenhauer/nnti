#!/usr/bin/perl -w
use strict;
use strict 'refs';
# ************************************************************************
# 
#                    Teuchos: Common Tools Package
#                 Copyright (2004) Sandia Corporation
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
#
# RAB: 2003/11/14: According the the documentation that I can find
# it looks like this script should be invoked from:
#
#    BUILD_DIR/packages/teuchos/test
#
# and therefore I will write all of my relative paths from that
#
printf
  "\n******************************".
  "\n*** Running Teuchos tests ****".
  "\n******************************\n";

my $success = 1;  # Boolean (false=0,true=nonzero)
my $result;       # success=0, failure=nonzero
$result = system ('./RefCountPtr/RefCountPtr_test.exe --quiet');
$success = 0 if ($result != 0);
$result = system ('./BLAS/BLAS_test.exe');
$success = 0 if ($result != 0);
exit ($success ? 0 : -1 );
