// -*- c++ -*-

// @HEADER
// ***********************************************************************
//
//              PyTrilinos: Python Interface to Trilinos
//                 Copyright (2005) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Bill Spotz (wfspotz@sandia.gov)
//
// ***********************************************************************
// @HEADER

// This documentation string will be the python help facility help
// string
%define %anasazi_docstring
"The Anasazi module allows access to The Trilinos package
Anasazi.  Use the python help() facility for local documentation
on classes and methods, or see the on-line documentation for more
in-depth information."
%enddef

// Define the module name, its package and documentation string
%module(package   = "PyTrilinos",
	autodoc   = "1",
	docstring = %anasazi_docstring) Anasazi

// Code within the percent-bracket delimiters is copied verbatim to
// the C++ wrapper source file.  Anything that is %include-ed later
// needs to be #include-ed here.
%{
// System includes
#include <sstream>

// Configuration includes
#include "PyTrilinos_config.h"

// Anasazi includes
#include "AnasaziVersion.cpp"
#include "AnasaziBasicSort.hpp"
#include "AnasaziOutputManager.hpp"
%}

// General ignore directives
%ignore *::operator=;
%ignore *::print;

// Auto-documentation feature
%feature("autodoc", "1");

// Rename directives
%rename (OutputManager_) Anasazi::OutputManager;

// C++ STL support
using namespace std;
%include "stl.i"

// Support for other Trilinos packages
//%import "Epetra.i"

/////////////////////////////
// Anasazi Version support //
/////////////////////////////
%include "AnasaziVersion.cpp"
%pythoncode %{
__version__ = Anasazi_Version().split()[2]
%}

///////////////////////////////
// Anasazi BasicSort support //
///////////////////////////////
%include "AnasaziBasicSort.hpp"

///////////////////////////////////
// Anasazi OutputManager support //
///////////////////////////////////
%include "AnasaziOutputManager.hpp"
