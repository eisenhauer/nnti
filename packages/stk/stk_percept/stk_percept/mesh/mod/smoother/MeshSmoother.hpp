/*--------------------------------------------------------------------*/
/*    Copyright 2003 Sandia Corporation.                              */
/*    Under the terms of Contract DE-AC04-94AL85000, there is a       */
/*    non-exclusive license for use of this work by or on behalf      */
/*    of the U.S. Government.  Export of this program may require     */
/*    a license from the United States Government.                    */
/*--------------------------------------------------------------------*/
#ifndef MeshSmoother_hpp
#define MeshSmoother_hpp

#include <stk_percept/Percept.hpp>
#if !defined(__IBMCPP__) 

#include <stk_percept/PerceptMesh.hpp>

#include <stk_percept/mesh/geometry/kernel/MeshGeometry.hpp>

#undef USE_CALLGRIND
//#define USE_CALLGRIND
#ifdef USE_CALLGRIND
#include "/usr/netpub/valgrind-3.6.0/include/valgrind/callgrind.h"
#endif

namespace stk {
  namespace percept {

    const double PS_DEF_UNT_BETA = 1e-8;
    const double PS_DEF_SUC_EPS = 1e-4;

    /// Abstract base class smoother
    class MeshSmoother
    {

    protected:
      PerceptMesh *m_eMesh;
      int innerIter;
      double gradNorm;
      int parallelIterations;
      stk::mesh::Selector *m_boundarySelector;
      MeshGeometry *m_meshGeometry;

    public:

      MeshSmoother(PerceptMesh *eMesh, 
                   stk::mesh::Selector *boundary_selector=0,
                   MeshGeometry *meshGeometry=0, 
                   int innerIter=100, double gradNorm = 1.e-8, int parallelIterations=20) : 
        m_eMesh(eMesh), innerIter(innerIter), gradNorm(gradNorm), parallelIterations(parallelIterations), 
        m_boundarySelector(boundary_selector), m_meshGeometry(meshGeometry)
      {
        if (m_meshGeometry) 
          m_meshGeometry->m_cache_classify_bucket_is_active = true;
      }

      /// @deprecated
      static int count_invalid_elements();

      static int parallel_count_invalid_elements(PerceptMesh *eMesh);

      void run( bool always_smooth=true, int debug=0);
      virtual void run_algorithm() = 0;

      static bool select_bucket(stk::mesh::Bucket& bucket, PerceptMesh *eMesh);
      bool get_fixed_flag(stk::mesh::Entity* node_ptr);
      int classify_node(stk::mesh::Entity& node, size_t& curveOrSurfaceEvaluator) const;


    };

  }
}

#endif
#endif
