/*
#@HEADER
# ************************************************************************
#
#                 Copyright (2002) Sandia Corporation
#
# Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
# license for use of this work by or on behalf of the U.S. Government.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# Questions? Contact Jonathan Hu (jhu@sandia.gov) or Ray Tuminaro 
# (rstumin@sandia.gov).
#
# ************************************************************************
#@HEADER
*/
/* ******************************************************************** */
/* See the file COPYRIGHT for a complete copyright notice, contact      */
/* person and disclaimer.                                               */
/* ******************************************************************** */
#include "mrtr_segment_bilinearquad.H"
#include "mrtr_interface.H"

/*----------------------------------------------------------------------*
 |  ctor (public)                                            mwgee 10/05|
 |  id               (in)  a unique segment id                          |
 |  nnode            (in)  number of nodes on this segment              |
 |  nodeId           (in)  unique node ids of nodes on this segment     |
 |                         nodeIds have to be sorted on input such that |
 |                         1D case: end nodes of segments first going   |
 |                                  through segment in mathematical     |
 |                                  positive sense. This is             |
 |                                  important to compute the direction  |
 |                                  of the outward normal of the segment|
 |                         2D case: corner nodes of segment first in    |
 |                                  counterclockwise order              |
 |                                  This is                             |
 |                                  important to compute the direction  |
 |                                  of the outward normal of the segment|
 *----------------------------------------------------------------------*/
MOERTEL::Segment_BiLinearQuad::Segment_BiLinearQuad(int id, int nnode, int* nodeId, int out) :
MOERTEL::Segment(id,nnode,nodeId,out)
{
  stype_ = MOERTEL::Segment::seg_BiLinearQuad;
}

/*----------------------------------------------------------------------*
 |  ctor (public)                                            mwgee 10/05|
 |  This constructor should not be used by the user, it is used         |
 |  internally together with Pack/Unpack for communication              |
 *----------------------------------------------------------------------*/
MOERTEL::Segment_BiLinearQuad::Segment_BiLinearQuad(int out) :
MOERTEL::Segment(out)
{
}

/*----------------------------------------------------------------------*
 |  copy-ctor (public)                                       mwgee 10/05|
 *----------------------------------------------------------------------*/
MOERTEL::Segment_BiLinearQuad::Segment_BiLinearQuad(MOERTEL::Segment_BiLinearQuad& old) :
MOERTEL::Segment(old)
{
  // all date lives in the base class and is copied in MOERTEL::Segment(old)
}

/*----------------------------------------------------------------------*
 | pack all data in this segment into a vector               mwgee 10/05|
 *----------------------------------------------------------------------*/
int* MOERTEL::Segment_BiLinearQuad::Pack(int* size)
{ 
  cout << "***ERR*** MOERTEL::Segment_BiLinearQuad::Pack:\n"
       << "***ERR*** not impl.\n"
       << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
  exit(EXIT_FAILURE);     
  return NULL;
}

/*----------------------------------------------------------------------*
 | unpack all data from a vector into this class             mwgee 10/05|
 *----------------------------------------------------------------------*/
bool MOERTEL::Segment_BiLinearQuad::UnPack(int* pack)
{ 
  cout << "***ERR*** MOERTEL::Segment_BiLinearQuad::UnPack:\n"
       << "***ERR*** not impl.\n"
       << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
  exit(EXIT_FAILURE);     
  return true;
}

/*----------------------------------------------------------------------*
 |  dtor (public)                                            mwgee 10/05|
 *----------------------------------------------------------------------*/
MOERTEL::Segment_BiLinearQuad::~Segment_BiLinearQuad()
{ 
  // data held in base class is destroyed by base class destructor
}

/*----------------------------------------------------------------------*
 |  clone this segment (public)                              mwgee 10/05|
 *----------------------------------------------------------------------*/
MOERTEL::Segment* MOERTEL::Segment_BiLinearQuad::Clone()
{ 
  MOERTEL::Segment_BiLinearQuad* newseg = new MOERTEL::Segment_BiLinearQuad(*this);
  return (newseg);
}

/*----------------------------------------------------------------------*
 |  << operator                                              mwgee 10/05|
 *----------------------------------------------------------------------*/
ostream& operator << (ostream& os, const MOERTEL::Segment_BiLinearQuad& seg)
{
  seg.Print(); 
  return os;
}

/*----------------------------------------------------------------------*
 | build an outward normal at a node adjacent to this        mwgee 10/05|
 *----------------------------------------------------------------------*/
bool MOERTEL::Segment_BiLinearQuad::LocalCoordinatesOfNode(int lid, double* xi)
{ 
  cout << "***ERR*** MOERTEL::Segment_BiLinearQuad::LocalCoordinatesOfNode:\n"
       << "***ERR*** not impl.\n"
       << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
  exit(EXIT_FAILURE);     
  return true;
}

/*----------------------------------------------------------------------*
 | build basis vectors and metric at given point xi          mwgee 10/05|
 *----------------------------------------------------------------------*/
double MOERTEL::Segment_BiLinearQuad::Metric(double* xi, double g[], double G[][3])
{ 
  cout << "***ERR*** MOERTEL::Segment_BiLinearQuad::Metric:\n"
       << "***ERR*** not impl.\n"
       << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
  exit(EXIT_FAILURE);     
  return 0.0;
}

/*----------------------------------------------------------------------*
 | build an outward normal at a node adjacent to this        mwgee 10/05|
 | returns allocated vector of length 3 with outward normal             |
 *----------------------------------------------------------------------*/
double* MOERTEL::Segment_BiLinearQuad::BuildNormal(double* xi)
{ 
  cout << "***ERR*** MOERTEL::Segment_BiLinearQuad::BuildNormal:\n"
       << "***ERR*** not impl.\n"
       << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
  exit(EXIT_FAILURE);     
  return NULL;
}

/*----------------------------------------------------------------------*
 | compute the length (Area) of this segment                 mwgee 10/05|
 *----------------------------------------------------------------------*/
double MOERTEL::Segment_BiLinearQuad::Area()
{ 
  cout << "***ERR*** MOERTEL::Segment_BiLinearQuad::Area:\n"
       << "***ERR*** not impl.\n"
       << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
  exit(EXIT_FAILURE);     
  return 0.0;
}
