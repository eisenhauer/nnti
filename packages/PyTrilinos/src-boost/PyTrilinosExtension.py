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
# Questions? Contact Bill Spotz (wfspotz@sandia.gov)
#
# ************************************************************************
# @HEADER

# System imports
from   distutils.core import *
from   distutils      import sysconfig
import os
import sys

# Local imports
from MakefileVariables import *

######################################################################

def makePyTrilinosExtensions(moduleName):
    """
    makePyTrilinosExtensions(str) -> list of distutils Extension objects

    The string argument is the capitalized form of the PyTrilinos module name.
    From this information (and the src directory Makefile), this function can
    determine all of the information necessary to build a list of distutils
    Extension objects that can be passed to the distutils setup() function.
    """

    # Various permutations of module name
    moduleNameLower = moduleName.lower()
    moduleNameUpper = moduleName.upper()
    includeVar      = moduleNameUpper + "_INCLUDES"
    libVar          = moduleNameUpper + "_LIBS"
    wrapperVar      = moduleNameUpper + "_WRAPPERS"
    exportVar       = moduleNameUpper + "_EXPORTS"

    # Parse and evaluate all of the Makefile variables in the current directory
    vars    = processMakefile("Makefile")
    srcdir  = vars["srcdir"]
    CXX     = vars["CXX"]
    exports = vars[exportVar].split()

    # Obtain the NumPy version
    from numpy import __version__ as numpyVersion

    # Compiler and linker.  This ensures that the MPI compilers are used when
    # appropriate.
    sysconfig.get_config_vars()
    sysconfig._config_vars["CC" ] = CXX
    sysconfig._config_vars["CXX"] = CXX

    # Define lists of various helper code
    epetraNumPyVectorSrc = [os.path.join(srcdir,"Epetra_NumPyIntVector.cpp"  ),
                            os.path.join(srcdir,"Epetra_NumPyMultiVector.cpp"),
                            os.path.join(srcdir,"Epetra_NumPyVector.cpp"     ),
                            os.path.join(srcdir,"Epetra_NumPyFEVector.cpp"   )]
    epetraNumPySerialDenseSrc = [os.path.join(srcdir,"Epetra_NumPyIntSerialDenseMatrix.cpp"),
                                 os.path.join(srcdir,"Epetra_NumPyIntSerialDenseVector.cpp"),
                                 os.path.join(srcdir,"Epetra_NumPySerialDenseMatrix.cpp"   ),
                                 os.path.join(srcdir,"Epetra_NumPySerialDenseVector.cpp"   )]
    epetraNumPySrc = epetraNumPyVectorSrc + epetraNumPySerialDenseSrc

    # Initialize Extension class constructor arguments
    define_macros      = [ ]
    include_dirs       = [srcdir]
    library_dirs       = ["."]
    libraries          = ["boost_python"]
    extra_compile_args = vars["CPPFLAGS"].split() + \
                         vars["CXXFLAGS"].split()
    uniquifyList(extra_compile_args)
    extra_link_args    = extra_compile_args[:]    # Shallow copy

    # Set all of the define macros
    define_macros.append(("HAVE_CONFIG_H","1"))
    if int(numpyVersion.split(".")[0]) > 0:
        define_macros.append(("NUMPY_NOPREFIX","1"))
    if moduleName == "ML":
        define_macros.append(("MLAPI_LC","1"))

    # Get the relevant Makefile export variable values, split them into lists of
    # strings, add them together to obtain a big list of option strings, and
    # then remove any duplicate entries
    options = vars[includeVar].split() + \
              vars[libVar].split()
    uniquifyList(options)

    # Distribute the individual options to the appropriate Extension class arguments
    for option in options:
        if option[:2] == "-I":
            include_dirs.append(option[2:])   # Omit the "-I" prefix
        elif option[:2] == "-L":
            library_dirs.append(option[2:])   # Omit the "-L" prefix
        elif option[:2] == "-l":
            libraries.append(option[2:])      # Omit the "-l" prefix
        elif option[-2:] == ".a":
            dir,lib = os.path.split(option)
            if dir: library_dirs.append(dir)
            libraries.append(lib[3:-2])       # Omit "lib" prefix and ".a" suffix
        else:
            extra_link_args.append(option)

    # Epetra needs the Teuchos source directory for the Teuchos_FILEstream.hpp
    # header.  It might already be there if thyra is enabled.
    if moduleName == "Epetra":
        teuchosSrcDir = os.path.abspath(os.path.join(srcdir, "..", "..",
                                                     "teuchos", "src"))
        if not (teuchosSrcDir in include_dirs):
            include_dirs.append(teuchosSrcDir)

    # Find the include directory for numpy.  Function get_numpy_include is
    # deprecated now in favor of get_include, but let's support the older
    # version and suppress the warning if we can.
    try:
        from numpy import get_include
        include_dirs.append(get_include())
    except ImportError:
        from numpy import get_numpy_include
        include_dirs.append(get_numpy_include())

    # Obtain a list of the boost wrapper files
    wrappers = vars[wrapperVar].split()

    # Create an Extension object for each wrapper file
    extensions = [ ]
    for wrapper in wrappers:

        # Generate the Extension name
        extName = ".".join(["PyTrilinos","_"+wrapper.replace("_wrap.cpp","")])

        # Build the list of sources
        sources = [os.path.join(srcdir,wrapper)] + exports
        if moduleName in ("Epetra",):
            sources.extend(epetraNumPySrc)
        if moduleName in ("EpetraExt","TriUtils","AztecOO","Galeri","IFPACK","Anasazi"):
            sources.extend(epetraNumPyVectorSrc)
        if extName in ("PyTrilinos.NOX.Epetra.___init__",
                       "PyTrilinos.NOX.Epetra._Interface"):
            sources.extend(epetraNumPyVectorSrc)

        # Create the Extension object and add it to the list
        extensions.append(Extension(extName,
                                    sources,
                                    define_macros      = define_macros,
                                    include_dirs       = include_dirs,
                                    library_dirs       = library_dirs,
                                    libraries          = libraries,
                                    extra_compile_args = extra_compile_args,
                                    extra_link_args    = extra_link_args
                                    )
                          )

    # All done -- return the list of extensions
    return extensions
