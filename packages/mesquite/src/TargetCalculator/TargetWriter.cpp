/* ***************************************************************** 
    MESQUITE -- The Mesh Quality Improvement Toolkit

    Copyright 2006 Sandia National Laboratories.  Developed at the
    University of Wisconsin--Madison under SNL contract number
    624796.  The U.S. Government and the University of Wisconsin
    retain certain rights to this software.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License 
    (lgpl.txt) along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 
    (2006) kraftche@cae.wisc.edu
   
  ***************************************************************** */


/** \file TargetWriter.cpp
 *  \brief 
 *  \author Jason Kraftcheck 
 */

#include "Mesquite.hpp"
#include "TargetWriter.hpp"
#include "TargetCalculator.hpp"
#include "WeightCalculator.hpp"
#include "MeshInterface.hpp"
#include "MsqMatrix.hpp"
#include "SamplePoints.hpp"
#include "PatchData.hpp"
#include "PatchSet.hpp"
#include "MsqError.hpp"
#include "ElementPatches.hpp"
#include "ElemSampleQM.hpp"
#ifdef MSQ_USE_OLD_IO_HEADERS
# include <sstream.h>
#else
# include <sstream>
#endif

namespace Mesquite {

TargetWriter::TargetWriter(  const SamplePoints* pts,
                             TargetCalculator* tc,
                             WeightCalculator* wc,
                             msq_std::string target_base_name,
                             msq_std::string weight_base_name )
  : samplePoints(pts),
    targetCalc(tc), 
    weightCalc(wc), 
    targetName(target_base_name), 
    weightName(weight_base_name)
    {}

TargetWriter::~TargetWriter() {}

msq_std::string TargetWriter::get_name() const
  { return "TargetWriter"; }

static void append_samples( msq_std::vector<Sample>& samples,
                            unsigned dim, unsigned num )
{
  size_t in_size = samples.size();
  samples.resize( in_size + num );
  for (unsigned i = 0; i < num; ++i)
    samples[i+in_size] = Sample( dim, i );
}

void TargetWriter::get_samples( EntityTopology type,
                                msq_std::vector<Sample>& samples )
{
  samples.clear();
  const int d = TopologyInfo::dimension(type);
  if (samplePoints->will_sample_at(type,0))
    append_samples( samples, 0, type == PYRAMID ? 4 : TopologyInfo::corners(type) );
  if (samplePoints->will_sample_at(type,1))
    append_samples( samples, 1, TopologyInfo::edges(type) );
  if (3 == d && samplePoints->will_sample_at(type,2))
    append_samples( samples, 2, TopologyInfo::faces(type) );
  if (samplePoints->will_sample_at(type,d))
    append_samples( samples, d, 1 );
}

double TargetWriter::loop_over_mesh( Mesh* mesh, 
                                     MeshDomain* domain, 
                                     const Settings* settings,
                                     MsqError& err )
{
  PatchData patch;
  patch.set_mesh( mesh );
  patch.set_domain( domain );
  patch.attach_settings( settings );
  
  ElementPatches patch_set;
  patch_set.set_mesh( mesh );
  msq_std::vector<PatchSet::PatchHandle> patches;
  msq_std::vector<PatchSet::PatchHandle>::iterator p;
  msq_std::vector<Mesh::VertexHandle> patch_verts;
  msq_std::vector<Mesh::ElementHandle> patch_elems;
  
  patch_set.get_patch_handles( patches, err ); MSQ_ERRZERO(err);
  
  msq_std::vector< MsqMatrix<3,3> > targets3d;
  msq_std::vector< MsqMatrix<3,2> > targets2d;
  msq_std::vector< double > weights;
  msq_std::vector< Sample > samples;
  for (p = patches.begin(); p != patches.end(); ++p)
  {
    patch_verts.clear();
    patch_elems.clear();
    patch_set.get_patch( *p, patch_elems, patch_verts, err ); MSQ_ERRZERO(err);
    patch.set_mesh_entities( patch_elems, patch_verts, err ); MSQ_ERRZERO(err);
    assert(patch.num_elements() == 1);
    
    MsqMeshEntity& elem = patch.element_by_index(0);
    EntityTopology type = elem.get_element_type();
    get_samples( type, samples );
    
    if (targetCalc) {
      const unsigned dim = TopologyInfo::dimension(type);
      if (dim == 2) {
        targets2d.resize( samples.size() );
        for (unsigned i = 0; i < samples.size(); ++i) {
          targetCalc->get_2D_target( patch, 0, samplePoints, samples[i], targets2d[i], err ); MSQ_ERRZERO(err);

          MsqMatrix<3,1> cross = targets2d[i].column(0) * targets2d[i].column(1);
          if (DBL_EPSILON > (cross%cross)) {
            MSQ_SETERR(err)("Degenerate 2D target", MsqError::INVALID_ARG);
            return 0.0;
          }
        }
        
        TagHandle tag = get_target_tag( 2, samples.size(), mesh, err ); MSQ_ERRZERO(err);
        mesh->tag_set_element_data( tag, 1, 
                                    patch.get_element_handles_array(), 
                                    &targets2d[0], err ); MSQ_ERRZERO(err);
      }
      else {
        targets3d.resize( samples.size() );
        for (unsigned i = 0; i < samples.size(); ++i) {
          targetCalc->get_3D_target( patch, 0, samplePoints, samples[i], targets3d[i], err ); MSQ_ERRZERO(err);

          if (DBL_EPSILON > det(targets3d[i])) {
            MSQ_SETERR(err)("Inverted 3D target", MsqError::INVALID_ARG);
            return 0.0;
          }
        }
        
        TagHandle tag = get_target_tag( 3, samples.size(), mesh,  err ); MSQ_ERRZERO(err);
        mesh->tag_set_element_data( tag, 1, 
                                    patch.get_element_handles_array(), 
                                    &targets3d[0], err ); MSQ_ERRZERO(err);
      }
    }
      
    if (weightCalc) {
      weights.resize( samples.size() );
      for (unsigned i = 0; i < samples.size(); ++i) {
        weights[i] = weightCalc->get_weight( patch, 0, samplePoints, samples[i], err ); MSQ_ERRZERO(err);
      }
      TagHandle tag = get_weight_tag( samples.size(), mesh, err ); MSQ_ERRZERO(err);
      mesh->tag_set_element_data( tag, 1, 
                                  patch.get_element_handles_array(),
                                  &weights[0], err ); MSQ_ERRZERO(err);
    }
  }

  return 0.0;
}

TagHandle TargetWriter::get_target_tag( unsigned dim, unsigned count, Mesh* mesh, MsqError& err )
{
  count *= 3*dim; // num doubles
  if (targetTags.size() <= count)
    targetTags.resize( count+1, 0 );
  if (!targetTags[count]) {
    targetTags[count] = get_tag_handle( targetName, count, mesh, err );
    if (MSQ_CHKERR(err)) {
      targetTags[count] = 0;
      return 0;
    }
  }
  return targetTags[count];
}

TagHandle TargetWriter::get_weight_tag( unsigned count, Mesh* mesh, MsqError& err )
{
  if (weightTags.size() <= count)
    weightTags.resize( count+1, 0 );
  if (!weightTags[count]) {
    weightTags[count] = get_tag_handle( weightName, count, mesh, err );
    if (MSQ_CHKERR(err)) {
      weightTags[count] = 0;
      return 0;
    }
  }
  return weightTags[count];
}

TagHandle TargetWriter::get_tag_handle( const msq_std::string& base_name,
                                        unsigned num_dbl, Mesh* mesh, MsqError& err )
{
  msq_stdio::ostringstream sstr;
  sstr << base_name << num_dbl;
  
  TagHandle handle = mesh->tag_get( sstr.str().c_str(), err );
  if (!MSQ_CHKERR(err))
  {
    msq_std::string temp_name;
    Mesh::TagType temp_type;
    unsigned temp_length;
    mesh->tag_properties( handle, temp_name, temp_type, temp_length, err );
    MSQ_ERRZERO(err);
    
    if (temp_type != Mesh::DOUBLE || temp_length != num_dbl)
    {
      MSQ_SETERR(err)( MsqError::TAG_ALREADY_EXISTS,
                      "Mismatched type or length for existing tag \"%s\"",
                       sstr.str().c_str() );
    }
  }
  else if (err.error_code() == MsqError::TAG_NOT_FOUND)
  {
    err.clear();
    handle = mesh->tag_create( sstr.str().c_str(), Mesh::DOUBLE, num_dbl, 0, err );
    MSQ_ERRZERO(err);
  }
  return handle;
}

} // namespace Mesquite
