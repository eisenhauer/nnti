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
#ifdef TRILINOS_PACKAGE

#include <ctime>
#include <vector>

#include "mrtr_interface.H"
#include "mrtr_utils.H"
#include "mrtr_pnode.H"
#include "mrtr_segment.H"
#include "mrtr_integrator.H"

#include "Epetra_SerialDenseMatrix.h"

/*----------------------------------------------------------------------*
 |  make mortar integration of this interface                           |
 *----------------------------------------------------------------------*/
bool MRTR::Interface::Mortar_Integrate(Epetra_CrsMatrix& D, 
                                       Epetra_CrsMatrix& M)
{ 
  bool ok = false;
  
  //-------------------------------------------------------------------
  // interface needs to be complete
  if (!IsComplete())
  {
    if (gcomm_.MyPID()==0)
      cout << "***ERR*** MRTR::Interface::Mortar_Integrate:\n"
           << "***ERR*** Complete() not called on interface " << Id_ << "\n"
           << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
    return false;
  }
  
  //-------------------------------------------------------------------
  // send all procs not member of this interface's intra-comm out of here
  // FIXME for testing, leave them in
  //if (!lComm()) return true;

  //-------------------------------------------------------------------
  // interface needs to have a mortar side assigned
  if (MortarSide()==-1)
  {
    if (gcomm_.MyPID()==0)
      cout << "***ERR*** MRTR::Interface::Mortar_Integrate:\n"
           << "***ERR*** mortar side was not assigned on interface " << Id_ << "\n"
           << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
    return false;
  }
  
  //-------------------------------------------------------------------
  // interface segments need to have at least one function on the mortar side
  // and two functions on the slave side
  int mside = MortarSide();
  int sside = OtherSide(mside);
  map<int,MRTR::Segment*>::iterator scurr;
  for (scurr=seg_[mside].begin(); scurr!=seg_[mside].end(); ++scurr)
    if (scurr->second->Nfunctions() < 1)
    {
      cout << "***ERR*** MRTR::Interface::Mortar_Integrate:\n"
           << "***ERR*** interface " << Id_ << ", mortar side\n"
           << "***ERR*** segment " << scurr->second->Id() << " needs at least 1 function set\n"
           << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
      return false;
    }
  for (scurr=seg_[sside].begin(); scurr!=seg_[sside].end(); ++scurr)
    if (scurr->second->Nfunctions() < 2)
    {
      cout << "***ERR*** MRTR::Interface::Mortar_Integrate:\n"
           << "***ERR*** interface " << Id_ << ", slave side\n"
           << "***ERR*** segment " << scurr->second->Id() << " needs at least 2 function set\n"
           << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
      return false;
    }
    
  //-------------------------------------------------------------------
  // do the integration of the master and slave side
  if (IsOneDimensional())
  {
    ok = Integrate_2D(M,D);
    if (!ok) return false;
  }
  else
  {
    cout << "***ERR*** MRTR::Interface::Mortar_Integrate:\n"
         << "***ERR*** Interface " << Id() << " 2D interface integration not yet impl.\n"
         << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
    exit(EXIT_FAILURE);
  }

  //-------------------------------------------------------------------
  // set the flag that this interface has been successfully integrated
  isIntegrated_ = true;
  
  return true;
}

/*----------------------------------------------------------------------*
 |  make mortar integration of master/slave side in 2D (1D interface)   |
 *----------------------------------------------------------------------*/
bool MRTR::Interface::Integrate_2D(Epetra_CrsMatrix& M,
                                   Epetra_CrsMatrix& D)
{ 
  if (!IsComplete())
  {
    if (gcomm_.MyPID()==0)
      cout << "***ERR*** MRTR::Interface::Integrate_MasterSide_2D:\n"
           << "***ERR*** Complete() not called on interface " << Id_ << "\n"
           << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
    return false;
  }
  if (!lComm()) return true;

  // get the sides
  int mside = MortarSide();
  int sside = OtherSide(mside);

  
  // loop over all segments of slave side
  map<int,MRTR::Segment*>::iterator scurr;
  for (scurr=rseg_[sside].begin(); scurr!=rseg_[sside].end(); ++scurr)
  {
    // the segment to be integrated
    MRTR::Segment* actsseg = scurr->second;

#if 0
    cout << "\nActive sseg id " << actsseg->Id() << "\n\n";
#endif

    // check whether I own at least one of the nodes on this slave segment
    int nnode = actsseg->Nnode();
    MRTR::Node** nodes = actsseg->Nodes();
    bool foundone = false;
    for (int i=0; i<nnode; ++i)
      if (NodePID(nodes[i]->Id()) == lComm()->MyPID())
      {
        foundone = true;
        break;
      }
    // if none of the nodes belongs to me, do nothing on this segment
    if (!foundone) continue;
    
    // loop over all segments on the master side
    map<int,MRTR::Segment*>::iterator mcurr;
    for (mcurr=rseg_[mside].begin(); mcurr!=rseg_[mside].end(); ++mcurr)    
    {
      MRTR::Segment* actmseg = mcurr->second;
      
#if 0
    cout << "Active mseg id " << actmseg->Id() << endl;
#endif
      // if there is an overlap, integrate the pair
      // (whether there is an overlap or not will be checked inside)
      Integrate_2D_Section(*actsseg,*actmseg,M,D);
      
    } // for (mcurr=rseg_[mside].begin(); mcurr!=rseg_[mside].end(); ++mcurr)  
  } // for (scurr=rseg_[sside].begin(); scurr!=rseg_[sside].end(); ++scurr)

  return true;
}


/*----------------------------------------------------------------------*
 | integrate the master/slave side's contribution from the overlap      |
 | of 2 segments (2D version) IF there is an overlap                    |
 *----------------------------------------------------------------------*/
bool MRTR::Interface::Integrate_2D_Section(MRTR::Segment& sseg, 
                                           MRTR::Segment& mseg,
                                           Epetra_CrsMatrix& M,
                                           Epetra_CrsMatrix& D)
{ 
  // if one of the segments is quadratic, we have to do something here
  if (sseg.Type()!=MRTR::Segment::seg_Linear1D || mseg.Type()!=MRTR::Segment::seg_Linear1D)
  {
    cout << "***ERR*** MRTR::Interface::Integrate_2D_Section:\n"
         << "***ERR*** Integration of other then linear segments not yet implemented\n"
         << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
    exit(EXIT_FAILURE);
  }

#if 0
  cout << "\n\nSlave Segment:\n";
  cout << sseg;
  cout << "Master Segment:\n";
  cout << mseg;
#endif

  // there is several cases on how these 2 segments can overlap
  // handle all of them, including the ones that they don't overlap 
  // at all
  
  // get slave and master's projections of the end points
  MRTR::Node** snodes = sseg.Nodes();
  MRTR::Node** mnodes = mseg.Nodes();

  bool snode0 = false;
  bool snode1 = false;
  bool mnode0 = false;
  bool mnode1 = false;
  int foundcase =  0;
  
  if (snodes[0]->GetProjectedNode())
    if (snodes[0]->GetProjectedNode()->Segment())
      if (snodes[0]->GetProjectedNode()->Segment()->Id() == mseg.Id())
        snode0 = true;
  if (snodes[1]->GetProjectedNode())
    if (snodes[1]->GetProjectedNode()->Segment())
      if (snodes[1]->GetProjectedNode()->Segment()->Id() == mseg.Id())
        snode1 = true;
      
  if (mnodes[0]->GetProjectedNode())
    if (mnodes[0]->GetProjectedNode()->Segment())
      if (mnodes[0]->GetProjectedNode()->Segment()->Id() == sseg.Id())
        mnode0 = true;
  if (mnodes[1]->GetProjectedNode())
    if (mnodes[1]->GetProjectedNode()->Segment())
      if (mnodes[1]->GetProjectedNode()->Segment()->Id() == sseg.Id())
        mnode1 = true;
      
  MRTR::ProjectedNode* nstart = NULL;
  MRTR::ProjectedNode* nend   = NULL;

  // the xi range to integrate
  double sxia=999.0,sxib=999.0;
  double mxia=999.0,mxib=999.0;
  
  // case 1: snodes don't project into master element and
  //         mnodes don't project into slave element
  if (!snode0 && !snode1 && !mnode0 && !mnode1)
  {
    //cout << "Case 1: no overlap\n";
    ++foundcase;
  }
  
  // case 2: snode0 projects into master element
  //         snode1 not projects into master element
  //         mnodes not project into slave element
  // Note: this case is due to tolerance in projection
  if (snode0 && !snode1 && !mnode0 && !mnode1)
    ++foundcase;
  
  // case 3: mnode0 projects into slave element element
  //         mnode1 not projects into slave element
  //         snodes don't project into master element
  // Note: this case is due to tolerance in projection
  if (!snode0 && !snode1 && mnode0 && !mnode1)
    ++foundcase;
  
  // case 4: mnode0 doe not project into slave element element
  //         mnode1 projects into slave element
  //         snodes don't project into master element
  // Note: this case is due to tolerance in projection
  if (!snode0 && !snode1 && !mnode0 && mnode1)
    ++foundcase;
  
  // case 5: mnodes do not project into slave element
  //        snode0 does not project into master element
  //        snode1 does project into master element
  // Note: this case might happen when mnode1 and snode0
  //       project exactly on an opposite node and are assigned
  //       an other element then this one
  if (!snode0 && snode1 && !mnode0 && !mnode1)
  {
    //cout << "Case 5: weirdo projection\n";
    bool ok = true;
    // Have to check whether snodes[0] has a projection 
    // (into a neighbor master segment) and whether that projection point is
    // low in xi range (should be -1.0)
    nstart = snodes[0]->GetProjectedNode(); // check whether a projection exists 
    if (!nstart) ok = false;
    if (ok) sxia = nstart->Xi()[0]; 
    if (sxia > -1.1 && sxia < -0.9) ok = true; // check whether projection is good
    else                            ok = false;  
    if (ok)
    {    
      nend   = snodes[1]->GetProjectedNode(); 
      sxia = -1.0;
      sxib =  1.0;
      mxia = snodes[1]->GetProjectedNode()->Xi()[0];
      mxib =  1.0;
      ++foundcase;
    }
  }

  // case 6: both master node project into slave segment
  if (mnode0 && mnode1)
  {
    ++foundcase;
    nstart = mnodes[0]->GetProjectedNode();
    nend   = mnodes[1]->GetProjectedNode();
    sxia = nend->Xi()[0];
    sxib = nstart->Xi()[0];
    mxia = -1.0;
    mxib = 1.0;
  }
  
  // case 7: both slave nodes project into master segment
  if (snode0 && snode1)
  {
    ++foundcase;
    nstart = snodes[0]->GetProjectedNode();
    nend   = snodes[1]->GetProjectedNode();
    sxia = -1.0;
    sxib =  1.0;
    mxia = nend->Xi()[0];
    mxib = nstart->Xi()[0];
  }

  // case 8: first slave node in master segment and first master node in slave segment
  if (snode0 && !snode1 && mnode0 && !mnode1)
  {
    ++foundcase;
    nstart = snodes[0]->GetProjectedNode();
    nend   = mnodes[0]->GetProjectedNode();
    sxia = -1.0;
    sxib = nend->Xi()[0];
    mxia = -1.0;
    mxib = nstart->Xi()[0];
  }

  // case 9: last slave node in master segment and last master node in slave segment
  if (snode1 && !snode0 && mnode1 && !mnode0)
  {
    ++foundcase;
    nstart = mnodes[1]->GetProjectedNode();
    nend   = snodes[1]->GetProjectedNode();
    sxia = nstart->Xi()[0];
    sxib = 1.0;
    mxia = nend->Xi()[0];
    mxib = 1.0;
  }

  if (foundcase != 1)
  {
    cout << "***ERR*** MRTR::Interface::Integrate_2D_Section:\n"
         << "***ERR*** # cases that apply here: " << foundcase << "\n"
         << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
    cout << "Slave :" << sseg;
    MRTR::Node** nodes = sseg.Nodes();
    cout << *nodes[0];
    cout << *nodes[1];
    cout << "Master:" << mseg;
    nodes = mseg.Nodes();
    cout << *nodes[0];
    cout << *nodes[1];
    exit(EXIT_FAILURE);
  }
  
  // there might be no overlap
  if (!nstart && !nend)
    return true;

#if 0  
  cout << "slave  xi range " << sxia << " - " << sxib << endl;
  cout << "master xi range " << mxia << " - " << mxib << endl;
#endif
  
  // FIXME: need to get the number of multipliers attached to the slave segment 
  //        when using discontinous lambda, lambdas are attached to segment!
  
  // create an integrator instance of some given order
  MRTR::Integrator integrator(5,IsOneDimensional());
  
  // do the integration of the master side
  Epetra_SerialDenseMatrix* Mdense = 
                            integrator.Integrate(sseg,sxia,sxib,mseg,mxia,mxib);
  
  // do the integration of the slave side
  Epetra_SerialDenseMatrix* Ddense = integrator.Integrate(sseg,sxia,sxib);
  
     // put results -Mdense into Epetra_CrsMatrix M
   // note the sign change for M here
  integrator.Assemble(*this,sseg,mseg,M,*Mdense);

   // put results Ddense into Epetra_CrsMatrix D
  integrator.Assemble(*this,sseg,D,*Ddense);

#if 1 // modification for curved interfaces from paper by B. Wohlmuth
  // do this modification for
  // linear elements
  // vector valued PDE (ndof=2, e.g. elasticity)
  // |delta n| != 0
  if (sseg.Type() == MRTR::Segment::seg_Linear1D && 
      mseg.Type() == MRTR::Segment::seg_Linear1D)
  if (snodes[0]->Nlmdof() == snodes[1]->Nlmdof() &&
      mnodes[0]->Ndof() == mnodes[1]->Ndof() &&
      snodes[0]->Nlmdof() == mnodes[0]->Ndof())
  {
    Epetra_SerialDenseMatrix* Mmod = NULL;
    
    // get the normal at slave nodes
    const double* n0 = snodes[0]->N();
    const double* n1 = snodes[1]->N();

    // build the tangential orthogonal to the normal
    double t[2][2];
    t[0][0] = -n0[1]; t[1][0] = -n1[1];
    t[0][1] =  n0[0]; t[1][1] =  n1[0];
    double n[2][2];
    n[0][0] =  n0[0]; n[1][0] =  n1[0]; 
    n[0][1] =  n0[1]; n[1][1] =  n1[1]; 
    
    // build delta values of normal and tangential
    double dn[2]; double dt[2];
    dn[0] = n0[0] - n1[0];  
    dn[1] = n0[1] - n1[1];  
    dt[0] = t[0][0] - t[1][0];
    dt[1] = t[0][1] - t[1][1];
    
    // build norm of dn. If it's zero, don't do anything
    bool doit = false;
    double delta = dn[0]*dn[0]+dn[1]*dn[1];
    if (abs(delta)>1.0e-9) doit = true;

    if (doit)
    {
      // do the integration of the modification of the master side
      // integral ( -0.5 * psi_12 * phi_k ) k=1,...,nnode_master 
      Epetra_SerialDenseMatrix* Mmod_scalar =
                        integrator.Integrate_2D_Mmod(sseg,sxia,sxib,mseg,mxia,mxib);

      // create an Epetra_SerialDenseMatrix of dimension (nsnode x nlmdof , nmnode x nmdof)
      int nsnode = sseg.Nnode();
      int nsdof  = snodes[0]->Nlmdof();
      int nmnode = mseg.Nnode();
      int nmdof  = mnodes[0]->Ndof();
      Mmod =  new Epetra_SerialDenseMatrix(nsnode*nsdof,nmnode*nmdof);

      // add modification values to Mmod
      for (int snode=0; snode<nsnode; ++snode)
        for (int sdof=0; sdof<nsdof; ++sdof)
        {
          double nt[2];
          nt[0] = n[snode][sdof] * dn[0] + t[snode][sdof] * dt[0];
          nt[1] = n[snode][sdof] * dn[1] + t[snode][sdof] * dt[1];
          for (int mnode=0; mnode<nmnode; ++mnode)
            for (int mdof=0; mdof<nmdof; ++mdof)
            {
              double val = nt[mdof] * (*Mmod_scalar)(mnode,0);
              (*Mmod)(snode*nsdof+sdof,mnode*nmdof+mdof) = val;
              //cout << *Mmod;
            }
        } // for (int sdof=0; sdof<nsdof; ++sdof)
      //cout << *Mmod;

#if 0  // verification of the expression by expressions given in paper
      Epetra_SerialDenseMatrix* Mmod2 = new Epetra_SerialDenseMatrix(nsnode*nsdof,nmnode*nmdof);
      // n1 dot n2
      double n1n2 = 0.0;
      for (int i=0; i<2; ++i) n1n2 += n[0][i]*n[1][i];
      // third row of n1 x n2
      double n1xn2 = n[0][0]*n[1][1] - n[0][1]*n[1][0];
      
      // slave 0 sdof 0 master 0 mdof 0 
      (*Mmod2)(0,0) = -(*Mmod_scalar)(0,0) * (1.0-n1n2);
      // slave 0 sdof 0 master 0 mdof 1
      (*Mmod2)(0,1) =  (*Mmod_scalar)(0,0) * n1xn2;
      // slave 0 sdof 0 master 1 mdof 0
      (*Mmod2)(0,2) = -(*Mmod_scalar)(1,0) * (1.0-n1n2);
      // slave 0 sdof 0 master 1 mdof 1
      (*Mmod2)(0,3) =  (*Mmod_scalar)(1,0) * n1xn2;
      // slave 0 sdof 1 master 0 mdof 0 
      (*Mmod2)(1,0) = -(*Mmod_scalar)(0,0) * n1xn2;
      // slave 0 sdof 1 master 0 mdof 1
      (*Mmod2)(1,1) = -(*Mmod_scalar)(0,0) * (1.0-n1n2);
      // slave 0 sdof 1 master 1 mdof 0
      (*Mmod2)(1,2) = -(*Mmod_scalar)(1,0) * n1xn2;
      // slave 0 sdof 1 master 1 mdof 1
      (*Mmod2)(1,3) = -(*Mmod_scalar)(1,0) * (1.0-n1n2);
      // slave 1 sdof 0 master 0 mdof 0
      (*Mmod2)(2,0) = -(*Mmod_scalar)(0,0) * (n1n2-1.0);
      // slave 1 sdof 0 master 0 mdof 1
      (*Mmod2)(2,1) =  (*Mmod_scalar)(0,0) * n1xn2;
      // slave 1 sdof 0 master 1 mdof 0
      (*Mmod2)(2,2) = -(*Mmod_scalar)(1,0) * (n1n2-1.0);
      // slave 1 sdof 0 master 1 mdof 1
      (*Mmod2)(2,3) =  (*Mmod_scalar)(1,0) * n1xn2;
      // slave 1 sdof 1 master 0 mdof 0
      (*Mmod2)(3,0) = -(*Mmod_scalar)(0,0) * n1xn2;
      // slave 1 sdof 1 master 0 mdof 1
      (*Mmod2)(3,1) = -(*Mmod_scalar)(0,0) * (n1n2-1.0);
      // slave 1 sdof 1 master 1 mdof 0
      (*Mmod2)(3,2) = -(*Mmod_scalar)(1,0) * n1xn2;
      // slave 1 sdof 1 master 1 mdof 1
      (*Mmod2)(3,3) = -(*Mmod_scalar)(1,0) * (n1n2-1.0);
      cout << *Mmod2;
      delete Mmod2; Mmod2 = NULL;
#endif

      //  assemble -Mmod into M
      integrator.Assemble_2D_Mod(*this,sseg,mseg,M,*Mmod);
      
      // tidy up 
      if (Mmod)        delete Mmod;        Mmod = NULL;
      if (Mmod_scalar) delete Mmod_scalar; Mmod_scalar = NULL;
    } // if (doit)
  } // if a lot of stuff
#endif

  
  // tidy up 
  if (Mdense) delete Mdense; Mdense = NULL;
  if (Ddense) delete Ddense; Ddense = NULL;

  return true;
}




#endif // TRILINOS_PACKAGE
