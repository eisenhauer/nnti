/*!
  \file   SmartLaplacianSmoother.cpp
  \brief  

  The SmartLaplacianSmoother Class is the concrete class
  that performs Smart Laplacian Smoothing

  \author Michael Brewer
  \author Thomas Leurent
  \date   2002-01-17
*/

#include "SmartLaplacianSmoother.hpp"
#include "LaplacianSmoother.hpp"
#include "MsqMessage.hpp"
#include "LInfTemplate.hpp"
#include "MeanRatioQualityMetric.hpp"


using namespace Mesquite;


#undef __FUNC__
#define __FUNC__ "SmartLaplacianSmoother::SmartLaplacianSmoother" 
SmartLaplacianSmoother::SmartLaplacianSmoother(ObjectiveFunction* obj_func,
                                               MsqError &err) 
{
  this->set_name("SmartLaplacianSmoother");
  
  set_patch_type(PatchData::ELEMENTS_ON_VERTEX_PATCH, err,1,1);MSQ_CHKERR(err);
  if(obj_func==NULL){
    edgeQM = new MeanRatioQualityMetric;
    defaultObjFunc = new LInfTemplate(edgeQM);
    objFunc=defaultObjFunc;
  }
  else{
    objFunc=obj_func;
    defaultObjFunc=NULL;
  }
  
}  
#undef __FUNC__
#define __FUNC__ "SmartLaplacianSmoother::~SmartLaplacianSmoother" 
SmartLaplacianSmoother::~SmartLaplacianSmoother() 
{
  if(defaultObjFunc!=NULL){
    delete edgeQM;
    delete defaultObjFunc;
  }
  
}    
  
#undef __FUNC__
#define __FUNC__ "SmartLaplacianSmoother::initialize" 
void SmartLaplacianSmoother::initialize(PatchData& /*pd*/, MsqError& /*err*/)
{
 
}

#undef __FUNC__
#define __FUNC__ "SmartLaplacianSmoother::initialize_mesh_iteration" 
void SmartLaplacianSmoother::initialize_mesh_iteration(PatchData &/*pd*/,
                                                  MsqError &/*err*/)
{
  //  cout << "- Executing SmartLaplacianSmoother::iteration_complete()\n";
}

#undef __FUNC__
#define __FUNC__ "SmartLaplacianSmoother::optimize_vertex_position"
/*! \todo Michael:  optimize_vertex_position is probably not implemented
  in an optimal way.  We used to use all of the vertices in
  the patch as 'adjacent' vertices.  Now we call get_adjacent_vertex_indices.
  We could use a VERTICES_ON_VERTEX type of patch or a global patch?
*/
void SmartLaplacianSmoother::optimize_vertex_positions(PatchData &pd, 
                                                MsqError &err)
{
    //default the laplacian smoother to 3 even for 2-d elements.
    //int dim = get_mesh_set()->space_dim();
  size_t dim = 3;
  MsqVertex* verts=pd.get_vertex_array(err);
    //variables for the function values.
  double orig_val=0;
  double mod_val=0;
    //compute the original function value and check validity
  bool valid_flag = objFunc->evaluate(pd,orig_val,err);
  // does the Laplacian smoothing
  MsqFreeVertexIndexIterator free_iter(&pd, err);
  free_iter.reset();
  free_iter.next();
    //m is the free vertex.
  size_t m=free_iter.value();
  std::vector<size_t> vert_indices;
  vert_indices.reserve(25);
    //get vertices adjacent to vertex m
  pd.get_adjacent_vertex_indices(m,vert_indices,err);
    //move vertex m
    //save the original position of the free vertex
  Vector3D orig_position(verts[m]);
    //smooth the patch
  centroid_smooth_mesh(pd, vert_indices.size(), vert_indices,
                       m, dim, err); MSQ_CHKERR(err);
    //snap vertex m to domain
  pd.snap_vertex_to_domain(m,err);
    //if the original function val was invalid, then we allow the move
    //But, if it wasn valid, we need to decide.
  if(valid_flag){
      //compute the new value
    valid_flag = objFunc->evaluate(pd,mod_val,err);
      //if the new value is worse the original OR if the new value is not
      //valid (we already know the original value was valid by above) then
      //we don't allow the move.
    if(!valid_flag || mod_val>orig_val){
        //move the vert back to where it was.
      verts[m]=orig_position;
        //PRINT_INFO("\norig = %f, new = %f, new valid = %d",orig_val,mod_val,valid_flag);
    }
    
  }
  
}
  
#undef __FUNC__
#define __FUNC__ "SmartLaplacianSmoother::terminate_mesh_iteration" 
void SmartLaplacianSmoother::terminate_mesh_iteration(PatchData &/*pd*/,
                                                 MsqError &/*err*/)
{
  //  cout << "- Executing SmartLaplacianSmoother::iteration_complete()\n";
}
  
#undef __FUNC__
#define __FUNC__ "SmartLaplacianSmoother::cleanup" 
void SmartLaplacianSmoother::cleanup()
{
  //  cout << "- Executing SmartLaplacianSmoother::iteration_end()\n";
}
  

