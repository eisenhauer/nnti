#ifndef stk_adapt_UniformRefiner_hpp
#define stk_adapt_UniformRefiner_hpp

#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <cmath>
#include <utility>
#include <math.h>
#include <map>
#include <set>
#include <vector>

#include <Shards_BasicTopologies.hpp>
#include <Shards_CellTopologyData.h>

#include <boost/tuple/tuple_io.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#include <stk_percept/stk_mesh.hpp>
#include <stk_percept/PerceptMesh.hpp>
#include <stk_adapt/UniformRefinerPattern.hpp>
#include <stk_adapt/Colorer.hpp>

#include <stk_adapt/NodeRegistry.hpp>

#include <stk_adapt/SubDimCell.hpp>

#define UNIFORM_REF_REMOVE_OLD_STD_SET 1
#define UNIFORM_REF_REMOVE_OLD_STD_VECTOR 0
#define UNIFORM_REF_REMOVE_OLD_BOOST_SET 0

#if UNIFORM_REF_REMOVE_OLD_BOOST_SET
#include <boost/unordered_set.hpp>
#endif


namespace stk {
  namespace adapt {

    using namespace stk::mesh;
    using std::vector;
    using std::map;
    using std::set;


#if UNIFORM_REF_REMOVE_OLD_STD_SET
    typedef std::set<Entity *> elements_to_be_destroyed_type;
#endif
#if UNIFORM_REF_REMOVE_OLD_STD_VECTOR
    typedef std::vector<Entity *> elements_to_be_destroyed_type;
#endif
#if UNIFORM_REF_REMOVE_OLD_BOOST_SET
    typedef boost::unordered_set<Entity *> elements_to_be_destroyed_type;
#endif


    /// e.g. UniformRefiner<shards::Hex<8>, shards::Tet<4> >
    //template<typename FromTopology, typename ToTopology>
#if 0
    class ParallelMeshModAlgorithm
    {
    public:
      virtual void planActions()=0;
      virtual void performActions()=0;
    protected:
      void helperFunction1();
      void helperFunction2();
      //...
    };
#endif

    //========================================================================================================================
    //========================================================================================================================
    //========================================================================================================================
    //template<class UniformRefinerPattern>
    class UniformRefiner //: ParallelMeshModAlgorithm
    {
    public:
      UniformRefiner(percept::PerceptMesh& eMesh, UniformRefinerPatternBase & bp, FieldBase *proc_rank_field=0);
      UniformRefiner(percept::PerceptMesh& eMesh, std::vector<UniformRefinerPatternBase *>&  bp, FieldBase *proc_rank_field=0);


      //UniformRefiner(percept::PerceptMesh& eMesh, UniformRefinerPattern<void, void, 0>& bp);
  
      void 
      doBreak();

      void 
      setRemoveOldElements(bool do_remove);

      bool 
      getRemoveOldElements();
      
      static BlockNamesType 
      getBlockNames(std::string& block_name);

      void 
      setIgnoreSideSets(bool ignore_sidesets) ;

      bool 
      getIgnoreSideSets();

    protected:
  
      //void checkParallelConsitency();

      unsigned
      doForAllElements(EntityRank rank, NodeRegistry::ElementFunctionPrototype function, vector< ColorerSetType >& elementColors, vector<NeededEntityType>& needed_entity_ranks,
                       bool only_count=false, bool doAllElements=true);

      void 
      createElementsAndNodesAndConnectLocal(unsigned irank,  UniformRefinerPatternBase* breakPattern, 
                   vector< ColorerSetType >& elementColors,   vector<NeededEntityType>& needed_entity_ranks,  vector<Entity *>& new_elements_pool);

      bool
      createNewNeededNodeIds(const CellTopologyData * const cell_topo_data, 
                             const Entity& element, vector<NeededEntityType>& needed_entity_ranks, NewSubEntityNodesType& nodes);

      void 
      removeOldElements(EntityRank rank, UniformRefinerPatternBase* breakPattern );

      void 
      removeOldElements( elements_to_be_destroyed_type& elements_to_be_destroyed);

      void 
      addOldElementsToPart(EntityRank rank, UniformRefinerPatternBase* breakPattern, unsigned *elementType = 0u);

      void 
      renameNewParts(EntityRank rank, UniformRefinerPatternBase* breakPattern);

      void 
      fixSurfaceAndEdgeSetNames(EntityRank rank, UniformRefinerPatternBase* breakPattern);

      void 
      fixElementSides();

      void 
      fixElementSides1();

      void 
      fixElementSides(EntityRank side_rank);

      void 
      fixElementSides1(EntityRank side_rank);

      void 
      checkFixElementSides(EntityRank side_rank, EntityRank elem_rank);

      void 
      buildElementSideDB(SubDimCellToDataMap& cell_2_data_map);

      void 
      trace_print(std::string msg);

      void 
      checkBreakPatternValidityAndBuildRanks(std::vector<EntityRank>& ranks);

    private:
      percept::PerceptMesh& m_eMesh;

      //UniformRefinerPatternBase & m_breakPattern;
      std::vector<UniformRefinerPatternBase *> m_breakPattern;

      NodeRegistry* m_nodeRegistry;
      FieldBase *m_proc_rank_field;
      bool m_doRemove;

      std::vector<EntityRank> m_ranks;
      bool m_ignoreSideSets;
    };

  }
}
#endif
