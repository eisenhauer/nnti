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

# System imports
from   distutils.core import *
from   distutils      import sysconfig
import commands
import os
import string
import sys

# Build the python library directory name and library name.  These will be used
# when the linker is invoked to create the python extensions.
pythonDir = [sysconfig.get_config_var('LIBPL'  )      ]
pythonLib = [sysconfig.get_config_var('LIBRARY')[3:-2]]

# Any information that needs to be transferred from the autotooled Makefile is
# written to file setup.txt using python syntax to define a dictionary.  The
# keys of this 'makeInfo' dictionary are variable names and the corresponding
# values represent the data that will be needed by this setup.py script.
try:
    f = open("setup.txt")
    makeInfo = f.readlines()
    f.close()
    makeInfo = eval(string.join(makeInfo))
except IOError:
    makeInfo = { }

# Certain directory paths are needed by setup.py.  srcDir is the path for the
# source directory, pakDir is the path for the Trilinos package directory,
#srcDir = os.path.join(makeInfo.get('srcdir',''), "src")
srcDir = makeInfo.get("srcdir","")
pakDir = os.path.split(srcDir)[0]

# Define the include paths required by various packages.  Each of these is
# defined as a list of a single or more strings.  Thus they can be added
# together to yield a list of multiple strings.
epetraInc = [os.path.join(pakDir, "epetra", "src")]

# Define the library search directories needed to link to various package
# libraries.  Each of these is defined as a list of a single string.  Thus they
# can be added together to yield a list of multiple strings.
epetraLibDir    = [os.path.join("..", "..", "src")]

# Define the library names for various packages.  Each of these is defined as a
# list of a single string.  Thus they can be added together to yield a list of
# multiple strings.
epetraLib    = ["epetra" ]

# Get the UNIX system name
sysName = os.uname()[0]

# Standard libraries.  This is currently a hack.  The library "stdc++" is added
# to the standard library list for a case where we know it needs it.
if sysName == "Linux":
    stdLibs = ["stdc++"]
else:
    stdLibs = [ ]

# Create the extra arguments list and complete the standard libraries list.  This
# is accomplished by looping over the arguments in LDFLAGS, FLIBS and LIBS and
# adding them to the appropriate list.
extraArgs = [ ]
libs = makeInfo.get('LDFLAGS','').split() + makeInfo.get('FLIBS','').split() + \
       makeInfo.get('LIBS'   ,'').split()
for lib in libs:
    if lib[:2] == "-l":
        stdLibs.append(lib[2:])
    else:
        extraArgs.append(lib)

# Define the strings that refer to the specified source files.  These should be
# combined together as needed in a list as the second argument to the Extension
# constructor.
wrapEpetra         = os.path.join(srcDir,"wrap_Epetra.cpp"        )
epetraNumPyVector  = os.path.join(srcDir,"Epetra_NumPyVector.cpp" )
epetraVectorHelper = os.path.join(srcDir,"Epetra_VectorHelper.cpp")
numPyArray         = os.path.join(srcDir,"NumPyArray.cpp"         )
numPyWrapper       = os.path.join(srcDir,"NumPyWrapper.cpp"       )

# Epetra extension module
_Epetra = Extension("PyTrilinos._Epetra",
                    [wrapEpetra,
                     epetraNumPyVector,
                     epetraVectorHelper,
                     numPyArray,
                     numPyWrapper],
                    include_dirs    = epetraInc,
                    library_dirs    = epetraLibDir + pythonDir,
                    libraries       = epetraLib + stdLibs + pythonLib,
                    extra_link_args = extraArgs
                    )

# PyTrilinos.Epetra setup
setup(name         = "PyTrilinos.Epetra",
      version      = "1.0",
      description  = "Python Interface to Trilinos Package Epetra",
      author       = "Bill Spotz",
      author_email = "wfspotz@sandia.gov",
      package_dir  = {"PyTrilinos" : srcDir},
      packages     = ["PyTrilinos"],
      ext_modules  = [ _Epetra  ]
      )
