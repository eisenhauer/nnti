#ifndef stk_adapt_RefinerPattern_Tet4_Tet4_N_sierra_hpp
#define stk_adapt_RefinerPattern_Tet4_Tet4_N_sierra_hpp


//#include "UniformRefinerPattern.hpp"
#include <stk_adapt/sierra_element/RefinementTopology.hpp>
#include <stk_adapt/sierra_element/StdMeshObjTopologies.hpp>

#include <stk_adapt/RefinerPattern_Tri3_Tri3_N.hpp>

#include <stk_adapt/Percept_MOAB_SimplexTemplateRefiner.hpp>

#include "UniformRefinerPattern_Line2_Line2_2_sierra.hpp"

/** NOTE: A lot of the following code is unfinished (greedy triangulation scheme).
 *  The call to triangulate_tet_generic has been replaced with a call to the
 *  MOAB SimplexTemplateRefiner as modified for Percept/Adapt.
 *
 *  We are leaving this unfinished code in place until the MOAB testing is verified.
 */

#define PERCEPT_USE_MOAB_REFINER 1

namespace stk {
  namespace adapt {

    /*---------------------------------------------------------------------*/
    /** From Shards_BasicTopologies.hpp
     *
     * typedef MakeTypeList< IndexList< 0 , 1 , 4 > ,
     *                       IndexList< 1 , 2 , 5 > ,
     *                       IndexList< 2 , 0 , 6 > ,
     *                       IndexList< 0 , 3 , 7 > ,
     *                       IndexList< 1 , 3 , 8 > ,
     *                       IndexList< 2 , 3 , 9 > >::type
     *   TetrahedronEdgeNodeMap ;
     *
     * typedef MakeTypeList< IndexList< 0 , 1 , 3 ,   4 , 8 , 7 > ,
     *                       IndexList< 1 , 2 , 3 ,   5 , 9 , 8 > ,
     *                       IndexList< 0 , 3 , 2 ,   7 , 9 , 6 > ,
     *                       IndexList< 0 , 2 , 1 ,   6 , 5 , 4 > >::type
     *   TetrahedronSideNodeMap ;
     *
     *
     */

    /*---------------------------------------------------------------------*/
    /**
     *                         PARENT 4-Node Tetrahedron Object Nodes
     *              3
     *              o
     *             /|\
     *            / | \       (PARENT) 4-Node Tetrahedron Object
     *           /  |  \               Edge Node Map:
     *          /   |   \         0      1       2       3       4       5
     *         /    |    \    { {0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 3}, {2, 3} };
     *      0 o-----|-----o 2
     *         \    |    /
     *          \   |   /         Face Node Map:
     *           \  |  /
     *            \ | /       { {0, 1, 3}, {1, 2, 3}, {0, 3, 2}, {0, 2, 1}  }
     *             \|/
     *              o
     *              1
     *
     *              3
     *              o
     *             /|\
     *            / | \
     *         7 *  |  * 9
     *          /   |   \
     *         /   6|    \
     *      0 o----*|-----o 2
     *         \    *8   /
     *          \   |   /
     *         4 *  |  * 5
     *            \ | /
     *             \|/
     *              o
     *              1
     */
    /*---------------------------------------------------------------------*/
    /** Edge (j) of face (i) maps to edge (k) of tet:  ==> tet_face_edge_map[4][3] == tet_face_edge_map[i][j]
     *
     *  face 0 {0, 1, 3}:  {0, 4, 3}
     *  face 1 {1, 2, 3}:  {1, 5, 4}
     *  face 2 {0, 3, 2}:  {3, 5, 2}
     *  face 3 {0, 2, 1}:  {2, 1, 0}
     */

    //static unsigned tbl_tet_face_edge_map[4][3]  = { {0, 4, 3}, {1, 5, 4}, {3, 5, 2}, {2, 1, 0} };
    //static unsigned tbl_tet_face_nodes[4][3]     = { {0, 1, 3}, {1, 2, 3}, {0, 3, 2}, {0, 2, 1} };
    //static unsigned tbl_tet_edge_nodes[6][2]     = { {0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 3}, {2, 3} };

    typedef boost::tuple<unsigned, unsigned, unsigned, unsigned> TetTupleTypeLocal;
    typedef boost::tuple<stk::mesh::EntityId, stk::mesh::EntityId, stk::mesh::EntityId, stk::mesh::EntityId> TetTupleType;

    /// general refinement pattern

    // the "-1" here signifies the number of elements created is not fixed, depends on the marking pattern
    template <>
    class RefinerPattern<shards::Tetrahedron<4>, shards::Tetrahedron<4>, -1 > : public URP<shards::Tetrahedron<4>,shards::Tetrahedron<4>  >
    {

      RefinerPattern<shards::Triangle<3>, shards::Triangle<3>, -1 > * m_face_breaker;

    public:

      RefinerPattern(percept::PerceptMesh& eMesh, BlockNamesType block_names = BlockNamesType()) :  URP<shards::Tetrahedron<4>, shards::Tetrahedron<4>  >(eMesh),
                                                                                                    m_face_breaker(0)
      {
        m_primaryEntityRank = stk::mesh::MetaData::ELEMENT_RANK;

        setNeededParts(eMesh, block_names, true);
        Elem::StdMeshObjTopologies::bootstrap();

        m_face_breaker =  new RefinerPattern<shards::Triangle<3>, shards::Triangle<3>, -1 > (eMesh, block_names) ;

      }

      ~RefinerPattern()
      {
        if (m_face_breaker) delete m_face_breaker;
      }

      void setSubPatterns( std::vector<UniformRefinerPatternBase *>& bp, percept::PerceptMesh& eMesh )
      {
        EXCEPTWATCH;
        bp = std::vector<UniformRefinerPatternBase *>(2u, 0);
        bp[0] = this;
        bp[1] = m_face_breaker;
      }

      virtual void doBreak() {}
      void fillNeededEntities(std::vector<NeededEntityType>& needed_entities)
      {
#if 0
        // FIXME - tmp, for now to create a centroid node which we delete later
        needed_entities.resize(2);
        needed_entities[0].first = m_eMesh.edge_rank();
        needed_entities[0].second = 1u;
        needed_entities[1].first = stk::mesh::MetaData::ELEMENT_RANK;
        needed_entities[1].second = 1u;
#else
        needed_entities.resize(1);
        needed_entities[0].first = m_eMesh.edge_rank();
        needed_entities[0].second = 1u;
#endif

      }

      // FIXME - for now, create more than we need (to fix this right we need a change to the Refiner.cpp interface)
      virtual unsigned getNumNewElemPerElem() { return 8; }

      /**
       *
       *   Convention: input is the element's nodes and the marks on the 6 edges.  Output is an array
       *     of "elements" defined as local id's of nodes forming those elements, where {0,1,2,3} represent
       *     the original vertices and {4,..,9} are the edges:
       *
       *
       *              3
       *              o
       *             /|\
       *            / | \
       *         7 *  |  * 9
       *          /   |   \
       *         /   6|    \
       *      0 o----*|-----o 2
       *         \    *8   /
       *          \   |   /
       *         4 *  |  * 5
       *            \ | /
       *             \|/
       *              o
       *              1
       */



#define TET_VERT_N(i) (i)
#define TET_EDGE_N(i) ((i)+4)

      static void triangulate_tet(PerceptMesh& eMesh, stk::mesh::Entity tet_elem_nodes[4], unsigned edge_marks[6],
                                  std::vector<TetTupleTypeLocal>& tets)
      {

        const CellTopologyData * const cell_topo_data = shards::getCellTopologyData< shards::Tetrahedron<4> >();

        shards::CellTopology cell_topo(cell_topo_data);
        //VectorFieldType* coordField = eMesh.get_coordinates_field();

        unsigned num_edges_marked=0;
        for (int iedge = 0; iedge < 6; iedge++)
          {
            unsigned num_nodes_on_edge = edge_marks[iedge];
            if (num_nodes_on_edge)
              {
                ++num_edges_marked;
              }
          }

        if (0)
          std::cout << "tmp RefinerPattern_Tet4_Tet4_N::num_edges_marked= " << num_edges_marked << std::endl;

        if (num_edges_marked == 0)
          {
            return;
          }
        // general case (now includes uniform refinement (all edges marked))
        else
          {
            double * node_coords[4];
            for (int inode=0; inode < 4; inode++)
              {
                node_coords[inode] = stk::mesh::field_data( *eMesh.get_coordinates_field() , tet_elem_nodes[inode] );
                if (0) std::cout << "tmp RP node_coords= "
                                 << node_coords[inode][0] << " "
                                 << node_coords[inode][1] << " "
                                 << node_coords[inode][2] << std::endl;
              }

            std::vector<moab::TetTupleInt> new_tets;
            bool choose_best_tets = true;
            moab::SimplexTemplateRefiner str(choose_best_tets);
            str.refine_3_simplex(new_tets, edge_marks, 1,
                                 node_coords[0], 0, tet_elem_nodes[0].identifier(),
                                 node_coords[1], 0, tet_elem_nodes[1].identifier(),
                                 node_coords[2], 0, tet_elem_nodes[2].identifier(),
                                 node_coords[3], 0, tet_elem_nodes[3].identifier() );

            if (0)
              {
                for (int inode=0; inode < 4; inode++)
                  {
                    node_coords[inode] = stk::mesh::field_data( *eMesh.get_coordinates_field() , tet_elem_nodes[inode] );
                    std::cout << "tmp RefPatt::createNewElements node_coords after= "
                              << node_coords[inode][0] << " "
                              << node_coords[inode][1] << " "
                              << node_coords[inode][2] << std::endl;
                  }
              }

            tets.resize(new_tets.size());
            for (unsigned i = 0; i < new_tets.size(); i++)
              {
                tets[i] = TetTupleTypeLocal((unsigned)new_tets[i].get<0>(),
                                            (unsigned)new_tets[i].get<1>(),
                                            (unsigned)new_tets[i].get<2>(),
                                            (unsigned)new_tets[i].get<3>() );
                if (0)
                  std::cout << "tmp RefPatt::createNewElements new tet= " << tets[i] << std::endl;

              }
          }
      }

      void
      createNewElements(percept::PerceptMesh& eMesh, NodeRegistry& nodeRegistry,
                        stk::mesh::Entity element,  NewSubEntityNodesType& new_sub_entity_nodes, std::vector<stk::mesh::Entity>::iterator& element_pool,
                        stk::mesh::FieldBase *proc_rank_field=0)
      {
        const CellTopologyData * const cell_topo_data = stk::percept::PerceptMesh::get_cell_topology(element);
        static std::vector<TetTupleType> elems(8);
        static std::vector<TetTupleTypeLocal> elems_local(8);
        unsigned num_new_elems=0;

        shards::CellTopology cell_topo(cell_topo_data);
        const stk::mesh::PairIterRelation elem_nodes = element.relations(stk::mesh::MetaData::NODE_RANK);
        //VectorFieldType* coordField = eMesh.get_coordinates_field();

        std::vector<stk::mesh::Part*> add_parts;
        std::vector<stk::mesh::Part*> remove_parts;
        add_parts = m_toParts;

        unsigned edge_marks[6] = {0,0,0,0,0,0};
        unsigned num_edges_marked=0;
        for (int iedge = 0; iedge < 6; iedge++)
          {
            unsigned num_nodes_on_edge = new_sub_entity_nodes[m_eMesh.edge_rank()][iedge].size();
            if (num_nodes_on_edge)
              {
                edge_marks[iedge] = 1;
                ++num_edges_marked;
              }
          }
        if (num_edges_marked == 0)
          return;

        stk::mesh::Entity elem_nodes_local[4] = {stk::mesh::Entity()};
        for (int inode=0; inode < 4; inode++)
          {
            elem_nodes_local[inode] = elem_nodes[inode].entity();
          }
        triangulate_tet(eMesh, elem_nodes_local, edge_marks, elems_local);

#define TET_CV_EV(i) ( i < 4 ? VERT_N(i) : EDGE_N(i-4) )

        num_new_elems = elems_local.size();
        elems.resize(num_new_elems);
        for (unsigned ielem=0; ielem < num_new_elems; ielem++)
          {
            elems[ielem] = TetTupleType( TET_CV_EV(elems_local[ielem].get<0>() ),
                                         TET_CV_EV(elems_local[ielem].get<1>() ),
                                         TET_CV_EV(elems_local[ielem].get<2>() ),
                                         TET_CV_EV(elems_local[ielem].get<3>() ) );
            if (0)
              std::cout << "tmp RefPatt::createNewElements new tet= " << elems[ielem] << std::endl;

          }

        //std::cout << "tmp RefinerPattern_Tet4_Tet4_N::num_edges_marked= " << num_edges_marked << std::endl;

        //nodeRegistry.makeCentroidCoords(*const_cast<stk::mesh::Entity>(&element), stk::mesh::MetaData::ELEMENT_RANK, 0u);

        for (unsigned ielem=0; ielem < elems.size(); ielem++)
          {
            stk::mesh::Entity newElement = *element_pool;

            if (proc_rank_field)
              {
                double *fdata = stk::mesh::field_data( *static_cast<const ScalarFieldType *>(proc_rank_field) , newElement );
                //fdata[0] = double(m_eMesh.get_rank());
                fdata[0] = double(newElement.owner_rank());
              }

            //eMesh.get_bulk_data()->change_entity_parts( newElement, add_parts, remove_parts );
            change_entity_parts(eMesh, element, newElement);

            {
              if (!elems[ielem].get<0>())
                {
                  std::cout << "P[" << eMesh.get_rank() << "] nid = 0 << " << std::endl;
                  //exit(1);
                }
            }

            // 4 nodes of the new tets
            eMesh.get_bulk_data()->declare_relation(newElement, eMesh.createOrGetNode(elems[ielem].get<0>()), 0);
            eMesh.get_bulk_data()->declare_relation(newElement, eMesh.createOrGetNode(elems[ielem].get<1>()), 1);
            eMesh.get_bulk_data()->declare_relation(newElement, eMesh.createOrGetNode(elems[ielem].get<2>()), 2);
            eMesh.get_bulk_data()->declare_relation(newElement, eMesh.createOrGetNode(elems[ielem].get<3>()), 3);

            set_parent_child_relations(eMesh, element, newElement, ielem);

            std::vector<stk::mesh::Entity> elements(1,element);
            interpolateElementFields(eMesh, elements, newElement);

            if (0)
              {
                std::cout << "tmp RefPatt::createNewElements element.identifier()= " << element.identifier()
                          << " newElement= " << newElement.identifier() << std::endl;

              }

            element_pool++;

          }


      }

    };

  }
}
#endif
