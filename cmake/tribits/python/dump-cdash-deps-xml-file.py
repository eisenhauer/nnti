#!/usr/bin/env python

# @HEADER
# ************************************************************************
#
#            TriBITS: Tribial Build, Integrate, and Test System
#                    Copyright 2013 Sandia Corporation
#
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the Corporation nor the names of the
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# ************************************************************************
# @HEADER


#
# Read in the commandline arguments
#

usageHelp = r"""dump-package-deps-table.py [OPTIONS]

Tool that dumps an XML file that can be read by CTest/CDash to specify the
Trilinos package dependenices in a way that is independent of Trilinos.  In
CTest/CDash terminology, a Trilinos package is a "Subproject".

By default, if you just run:

   $ SOME_DIR/dump-cdash-deps-xml-file.py

then the XML file will get written into the main Trilinos source directory
where it can be checked in on the next checkin.

You can also change what XML input file is used and what XML file is written.
This is maily to facilitate unit testing of this script.

Have fun looking through all of the Trilinos dependencies!

"""

from optparse import OptionParser

clp = OptionParser(usage=usageHelp)

clp.add_option(
  "--input-xml-deps-file", dest="inputXmlDepsFile", type="string",
  help="Input XML file giving the Trilinos dependencies.")

clp.add_option(
  "--output-cdash-deps-xml-file", dest="outputCDashDepsXmlFile", type="string",
  help="Output XML file giving the TriBITS project dependices in XML form that CDash understands.")

(options, args) = clp.parse_args()


#
# Execute the commands
#


from TribitsDependencies import getProjectDependenciesFromXmlFile

tribitsDependencies = getProjectDependenciesFromXmlFile(
  options.inputXmlDepsFile)

tribitsDependencies.writeCDashXmlDepsFile(
  options.outputCDashDepsXmlFile)
