#ifndef stk_adapt_TestLocalRefinerTri_N_3_MeshSizeRatio_hpp
#define stk_adapt_TestLocalRefinerTri_N_3_MeshSizeRatio_hpp

#include <stk_adapt/IElementAdapter.hpp>

#include <stk_mesh/base/GetBuckets.hpp>

namespace stk {
  namespace adapt {

    void exact_nodal_solution(const double *xyz, double *field, const int spatial_dim)
    {
      // 1d case for now
      const double xdiff = xyz[0] - 0.5329;
      const double lambda = 0.5;
      const double amp = 1e-2;
      const double twopi = 8.0*atan(1.0);

      if (xdiff > 0.0 && xdiff < 0.5)
	field[0] = 1.0 + amp*(1.0-cos(twopi*xdiff/lambda));
      else
	field[0] = 0.0;
    }

    double triangle_area(const std::vector<double> & nodal_coords)
    {
      double x[3] = {nodal_coords[0], nodal_coords[2], nodal_coords[4]};
      double y[3] = {nodal_coords[1], nodal_coords[3], nodal_coords[5]};

      return 0.5*fabs( (x[0]-x[2])*(y[1]-y[0]) - (x[0]-x[1])*(y[2]-y[0]) );
    }

    void compute_elem_mesh_size_ratio(
      PerceptMesh& eMesh,
      ScalarFieldType* elem_ratio_field,
      const double &global_error_tol)
    {
      const int spatial_dim = eMesh.get_spatial_dim();
      double local_error_tol = global_error_tol;

      static bool first_run = true;

      stk::mesh::Part * activeElementsPart = eMesh.get_non_const_part("refine_active_elements_part");

      stk::mesh::Selector selector = first_run  ?
	eMesh.get_fem_meta_data()->locally_owned_part() :
	( eMesh.get_fem_meta_data()->locally_owned_part() & (*activeElementsPart) );

      first_run = false;

      std::vector<unsigned> count ;
      stk::mesh::count_entities( selector, *eMesh.get_bulk_data(), count );

      const double num_elems = (double) count[stk::mesh::MetaData::ELEMENT_RANK];
      local_error_tol /= sqrt(num_elems);

      std::vector<stk::mesh::Bucket*> buckets;
      stk::mesh::get_buckets( selector, eMesh.get_bulk_data()->buckets( stk::mesh::MetaData::ELEMENT_RANK ), buckets );

      for ( vector<stk::mesh::Bucket*>::const_iterator k = buckets.begin() ; k != buckets.end() ; ++k ) {

	stk::mesh::Bucket & bucket = **k ;

	shards::CellTopology ct = stk::mesh::get_cell_topology(bucket);
	const int Nnpe = ct.getNodeCount();

	std::vector<double> nodal_interp(Nnpe);
	std::vector<double> nodal_coords(Nnpe*spatial_dim);

	const unsigned num_elems_in_bucket = bucket.size();
	for (unsigned i = 0; i < num_elems_in_bucket; i++) {

	  stk::mesh::Entity element = bucket[i];

	  // gather nodal coords and compute centroid
	  std::vector<double> centroid(spatial_dim, 0.0);
	  stk::mesh::PairIterRelation elem_nodes = element.relations(stk::mesh::MetaData::NODE_RANK);
	  for (unsigned inode=0; inode < elem_nodes.size(); inode++) {
	    stk::mesh::Entity node = elem_nodes[inode].entity();
	    double *coords = stk::mesh::field_data( *eMesh.get_coordinates_field() , node);

	    for (int d=0; d<spatial_dim; d++) {
	      centroid[d] += coords[d];
	      nodal_coords[inode*spatial_dim+d] = coords[d];
	    }

	    exact_nodal_solution(coords, &nodal_interp[inode], spatial_dim);
	  }

	  for (int d=0; d<spatial_dim; d++) {
	    centroid[d] /= (double) Nnpe;
	  }

	  // calc interpolation error at midpoint
	  double eval_centroid;
	  exact_nodal_solution(&centroid[0], &eval_centroid, spatial_dim);

	  double interp_centroid = 0.0;
	  for (unsigned inode=0; inode < elem_nodes.size(); inode++) {
	    interp_centroid += nodal_interp[inode];
	  }
	  interp_centroid /= (double) Nnpe;

	  const double err_centroid = eval_centroid - interp_centroid;

	  // HACK triangles for now
	  const double area = triangle_area(nodal_coords);

	  const double local_error = fabs(err_centroid) * area;

	  double *ratio = stk::mesh::field_data( *elem_ratio_field , element);

	  // calc elem ratio
	  *ratio = sqrt(local_error / local_error_tol);
	}
      }
    }

    //========================================================================================================================
    //========================================================================================================================
    //========================================================================================================================
    /**
     * A test implementation as a use case for MeshSizeRatio
     */
    class TestLocalRefinerTri_N_3_MeshSizeRatio : public IElementAdapter
    {
    public:
      TestLocalRefinerTri_N_3_MeshSizeRatio(
        percept::PerceptMesh& eMesh,
	UniformRefinerPatternBase & bp,
	ScalarFieldType * elem_ratio_field,
	stk::mesh::FieldBase *proc_rank_field=0);

    protected:

      /// Client supplies these methods - given an element, which edge,
      // and the nodes on the edge, return instruction on what to do to the edge,
      ///    DO_NOTHING (nothing), DO_REFINE (refine), DO_UNREFINE
      virtual int markElement(const stk::mesh::Entity element);

      ScalarFieldType * m_elem_ratio_field;
      const double m_Rup;
    };

    // This is a very specialized test that is used in unit testing only

    TestLocalRefinerTri_N_3_MeshSizeRatio::TestLocalRefinerTri_N_3_MeshSizeRatio(
      percept::PerceptMesh& eMesh,
      UniformRefinerPatternBase &  bp,
      ScalarFieldType * elem_ratio_field,
      stk::mesh::FieldBase *proc_rank_field)
      :
      IElementAdapter(eMesh, bp, proc_rank_field),
      m_elem_ratio_field(elem_ratio_field),
      m_Rup(sqrt(2.0))
    {}

    int
    TestLocalRefinerTri_N_3_MeshSizeRatio::markElement(const stk::mesh::Entity element)
    {
      int mark=0;

      // TEST simplest case: ratio > Rup
      const double & ratio = *( stk::mesh::field_data( *m_elem_ratio_field , element) );

      if (ratio > m_Rup)
	mark |= DO_REFINE;

      return mark;
    }
  }
}
#endif
