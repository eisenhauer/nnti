#ifndef stk_adapt_UniformRefinerPattern_Line2_Line3_1_sierra_hpp
#define stk_adapt_UniformRefinerPattern_Line2_Line3_1_sierra_hpp


//#include "UniformRefinerPattern.hpp"
#include <stk_adapt/sierra_element/RefinementTopology.hpp>
#include <stk_adapt/sierra_element/StdMeshObjTopologies.hpp>

#include <stk_percept/PerceptBoostArray.hpp>


namespace stk {
  namespace adapt {

    template <>
    class UniformRefinerPattern< shards::Line<2>, shards::Line<3>, 1, SierraPort > : public URP<shards::Line<2> , shards::Line<3> >
    {

    public:

      UniformRefinerPattern(percept::PerceptMesh& eMesh, BlockNamesType block_names = BlockNamesType()) : URP<shards::Line<2> , shards::Line<3> >(eMesh)
      {
        m_primaryEntityRank = m_eMesh.edge_rank();
        if (m_eMesh.get_spatial_dim() == 1)
          m_primaryEntityRank = stk::mesh::MetaData::ELEMENT_RANK;

        setNeededParts(eMesh, block_names, false);
        Elem::StdMeshObjTopologies::bootstrap();

      }


      virtual void doBreak() {}

      void setSubPatterns( std::vector<UniformRefinerPatternBase *>& bp, percept::PerceptMesh& eMesh )
      {
        EXCEPTWATCH;
        bp = std::vector<UniformRefinerPatternBase *>(1u, 0);

        if (eMesh.get_spatial_dim() == 1)
          {
            bp[0] = this;
          }
        else
          {
          }

      }

      void fillNeededEntities(std::vector<NeededEntityType>& needed_entities)
      {
        needed_entities.resize(1);
        //needed_entities[0].first = m_eMesh.edge_rank();
        needed_entities[0].first = (m_eMesh.get_spatial_dim() == 1 ? stk::mesh::MetaData::ELEMENT_RANK : m_eMesh.edge_rank());
        setToOne(needed_entities);
      }

      virtual unsigned getNumNewElemPerElem() { return 1; }

      void
      createNewElements(percept::PerceptMesh& eMesh, NodeRegistry& nodeRegistry,
                        stk::mesh::Entity element,  NewSubEntityNodesType& new_sub_entity_nodes, vector<stk::mesh::Entity>::iterator& element_pool,
                        stk::mesh::FieldBase *proc_rank_field=0)
      {
        const CellTopologyData * const cell_topo_data = stk::percept::PerceptMesh::get_cell_topology(element);
        typedef boost::array<unsigned,3> quadratic_type;
        static vector<quadratic_type> elems(1);

        CellTopology cell_topo(cell_topo_data);
        const percept::MyPairIterRelation elem_nodes (m_eMesh, element, stk::mesh::MetaData::NODE_RANK);

        std::vector<stk::mesh::Part*> add_parts;
        std::vector<stk::mesh::Part*> remove_parts;

        add_parts = m_toParts;

#if 0
        double coord_x[3];
        for (int iedge = 0; iedge < 1; iedge++)
          {
            double * mp = midPoint(VERT_COORD(0), VERT_COORD(1), eMesh.get_spatial_dim(), coord_x);
            //double * mp = midPoint(EDGE_COORD(iedge,0), EDGE_COORD(iedge,1), eMesh.get_spatial_dim(), coord_x);

            if (!EDGE_N(iedge))
              {
                std::cout << "P[" << eMesh.get_rank() << " nid ## = 0  " << std::endl;
              }
            eMesh.createOrGetNode(EDGE_N(iedge), mp);
          }
        // FIXME
        nodeRegistry.makeCentroidCoords(element, m_primaryEntityRank, 0u);
        nodeRegistry.interpolateFields(element, m_primaryEntityRank, 0u);
#endif
        nodeRegistry.addToExistingParts(element, m_primaryEntityRank, 0u);

#define CENTROID_N NN(m_primaryEntityRank,0)


        {
          quadratic_type& EN = elems[0];

          for (unsigned ind = 0; ind < 2; ind++)
            {
              unsigned inode = VERT_N(ind);
              EN[ind] = inode;
            }

          EN[2] = CENTROID_N;
        }

#undef CENTROID_N

        // write a diagram of the refinement pattern as a vtk file, or a latex/tikz/pgf file
#define WRITE_DIAGRAM 0
#if WRITE_DIAGRAM

#endif

        for (unsigned ielem=0; ielem < elems.size(); ielem++)
          {
            stk::mesh::Entity newElement = *element_pool;

            // FIXME
            if (0 && proc_rank_field)
              {
                double *fdata = stk::mesh::field_data( *static_cast<const ScalarFieldType *>(proc_rank_field) , newElement );
                fdata[0] = double(eMesh.owner_rank(newElement));
              }

            change_entity_parts(eMesh, element, newElement);


            {
              if (!elems[ielem][0])
                {
                  std::cout << "P[" << eMesh.get_rank() << " nid = 0  " << std::endl;
                  exit(1);
                }

            }
            for (int inode=0; inode < 3; inode++)
              {
                stk::mesh::EntityId eid = elems[ielem][inode];
                stk::mesh::Entity node = eMesh.createOrGetNode(eid);
                eMesh.get_bulk_data()->declare_relation(newElement, node, inode);
              }

            set_parent_child_relations(eMesh, element, newElement, ielem);

            element_pool++;

          }

      }

    };

  }
}
#endif
