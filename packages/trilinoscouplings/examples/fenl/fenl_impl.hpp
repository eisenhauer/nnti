/*
//@HEADER
// ************************************************************************
//
//   Kokkos: Manycore Performance-Portable Multidimensional Arrays
//              Copyright (2012) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact  H. Carter Edwards (hcedwar@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef KOKKOS_EXAMPLE_FENL_IMPL_HPP
#define KOKKOS_EXAMPLE_FENL_IMPL_HPP

#include <math.h>

// Kokkos libraries' headers:

#include <Kokkos_UnorderedMap.hpp>
#include <Kokkos_StaticCrsGraph.hpp>
#include <Kokkos_CrsMatrix.hpp>
#include <impl/Kokkos_Timer.hpp>

#include <Teuchos_CommHelpers.hpp>

// Examples headers:

#include <BoxElemFixture.hpp>
#include <VectorImport.hpp>
#include <CGSolve.hpp>

#include <fenl.hpp>
#include <fenl_functors.hpp>
#include <Kokkos_DefaultNode.hpp>
#include <Tpetra_Vector.hpp>
#include <Tpetra_CrsMatrix.hpp>

//----------------------------------------------------------------------------

namespace Kokkos {
namespace Example {
namespace FENL {

inline
double maximum( const Teuchos::RCP<const Teuchos::Comm<int> >& comm , double local )
{
  double global = 0 ;
  Teuchos::reduceAll( *comm , Teuchos::REDUCE_MAX , 1 , & local , & global );
  return global ;
}

} /* namespace FENL */
} /* namespace Example */
} /* namespace Kokkos */

//----------------------------------------------------------------------------

namespace Kokkos {
namespace Example {
namespace FENL {

class ManufacturedSolution {
public:

  // Manufactured solution for one dimensional nonlinear PDE
  //
  //  -K T_zz + T^2 = 0 ; T(zmin) = T_zmin ; T(zmax) = T_zmax
  //
  //  Has an analytic solution of the form:
  //
  //    T(z) = ( a ( z - zmin ) + b )^(-2) where K = 1 / ( 6 a^2 )
  //
  //  Given T_0 and T_L compute K for this analytic solution.
  //
  //  Two analytic solutions:
  //
  //    Solution with singularity:
  //    , a( ( 1.0 / sqrt(T_zmax) + 1.0 / sqrt(T_zmin) ) / ( zmax - zmin ) )
  //    , b( -1.0 / sqrt(T_zmin) )
  //
  //    Solution without singularity:
  //    , a( ( 1.0 / sqrt(T_zmax) - 1.0 / sqrt(T_zmin) ) / ( zmax - zmin ) )
  //    , b( 1.0 / sqrt(T_zmin) )

  const double zmin ;
  const double zmax ;
  const double T_zmin ;
  const double T_zmax ;
  const double a ;
  const double b ;
  const double K ;

  ManufacturedSolution( const double arg_zmin ,
                        const double arg_zmax ,
                        const double arg_T_zmin ,
                        const double arg_T_zmax )
    : zmin( arg_zmin )
    , zmax( arg_zmax )
    , T_zmin( arg_T_zmin )
    , T_zmax( arg_T_zmax )
    , a( ( 1.0 / sqrt(T_zmax) - 1.0 / sqrt(T_zmin) ) / ( zmax - zmin ) )
    , b( 1.0 / sqrt(T_zmin) )
    , K( 1.0 / ( 6.0 * a * a ) )
    {}

  double operator()( const double z ) const
  {
    const double tmp = a * ( z - zmin ) + b ;
    return 1.0 / ( tmp * tmp );
  }
};

} /* namespace FENL */
} /* namespace Example */
} /* namespace Kokkos */

//----------------------------------------------------------------------------

namespace Kokkos {
namespace Example {
namespace FENL {

template < class Device , BoxElemPart::ElemOrder ElemOrder >
Perf fenl(
  const ::Teuchos::RCP<const Teuchos::Comm<int> >& comm ,
  const int use_print ,
  const int use_trials ,
  const int use_atomic ,
  const int use_nodes[] )
{
  typedef typename ::Kokkos::Compat::KokkosDeviceWrapperNode<Device> NodeType;
  typedef typename ::Tpetra::CrsMatrix<double,int,int,NodeType> GlobalMatrixType;
  typedef typename ::Tpetra::Vector<double,int,int,NodeType> GlobalVectorType;
  typedef typename ::Tpetra::Map<int, int, NodeType> MapType;
  typedef typename ::Teuchos::RCP<const MapType> pMapType;

  typedef Kokkos::Example::BoxElemFixture< Device , ElemOrder > FixtureType ;

  //typedef Kokkos::CrsMatrix< double , unsigned , Device >
  typedef typename GlobalMatrixType::k_local_matrix_type
    LocalMatrixType ;

  typedef typename LocalMatrixType::StaticCrsGraphType
    LocalGraphType ;

  typedef Kokkos::Example::FENL::NodeNodeGraph< typename FixtureType::elem_node_type , LocalGraphType , FixtureType::ElemNode >
     NodeNodeGraphType ;

  typedef Kokkos::Example::FENL::ElementComputation< FixtureType , LocalMatrixType >
    ElementComputationType ;

  typedef Kokkos::Example::FENL::DirichletComputation< FixtureType , LocalMatrixType >
    DirichletComputationType ;

  typedef NodeElemGatherFill< ElementComputationType >
    NodeElemGatherFillType ;

  typedef typename ElementComputationType::vector_type LocalVectorType ;
  typedef Kokkos::DualView< double** , Kokkos::LayoutLeft, Device > LocalDualVectorType;
  typedef Kokkos::Example::VectorImport<
     typename FixtureType::comm_list_type ,
     typename FixtureType::send_nodeid_type ,
     LocalVectorType > ImportType ;


  //------------------------------------

  const unsigned newton_iteration_limit     = 10 ;
  const double   newton_iteration_tolerance = 1e-7 ;
  const unsigned cg_iteration_limit         = 200 ;
  const double   cg_iteration_tolerance     = 1e-7 ;

  //------------------------------------

  const int print_flag = use_print && Kokkos::Impl::is_same< Kokkos::HostSpace , typename Device::memory_space >::value ;

  const int comm_rank = comm->getRank();
  const int comm_size = comm->getSize();

  // Decompose by node to avoid mpi-communication for assembly

  const float bubble_x = 1.0 ;
  const float bubble_y = 1.0 ;
  const float bubble_z = 1.0 ;

  const FixtureType fixture( BoxElemPart::DecomposeNode , comm_size , comm_rank ,
                             use_nodes[0] , use_nodes[1] , use_nodes[2] ,
                             bubble_x , bubble_y , bubble_z );

  //------------------------------------

  const ImportType comm_nodal_import(
    comm ,
    fixture.recv_node() ,
    fixture.send_node() ,
    fixture.send_nodeid() ,
    fixture.node_count_owned() ,
    fixture.node_count() - fixture.node_count_owned() );

  //------------------------------------

  const double bc_lower_value = 1 ;
  const double bc_upper_value = 2 ;

  const Kokkos::Example::FENL::ManufacturedSolution
    manufactured_solution( 0 , 1 , bc_lower_value , bc_upper_value  );

  //------------------------------------

  if ( print_flag ) {
    std::cout << "Manufactured solution"
              << " a[" << manufactured_solution.a << "]"
              << " b[" << manufactured_solution.b << "]"
              << " K[" << manufactured_solution.K << "]"
              << " {" ;
    for ( unsigned inode = 0 ; inode < fixture.node_count() ; ++inode ) {
      std::cout << " " << manufactured_solution( fixture.node_coord( inode , 2 ) );
    }
    std::cout << " }" << std::endl ;

    std::cout << "ElemNode {" << std::endl ;
    for ( unsigned ielem = 0 ; ielem < fixture.elem_count() ; ++ielem ) {
      std::cout << "  elem[" << ielem << "]{" ;
      for ( unsigned inode = 0 ; inode < FixtureType::ElemNode ; ++inode ) {
        std::cout << " " << fixture.elem_node(ielem,inode);
      }
      std::cout << " }" << std::endl ;
    }
    std::cout << "}" << std::endl ;
  }

  //------------------------------------

  Kokkos::Impl::Timer wall_clock ;

  Perf perf_stats = Perf() ;

  for ( int itrial = 0 ; itrial < use_trials ; ++itrial ) {

    Perf perf = Perf() ;

    perf.global_elem_count = fixture.elem_count_global();
    perf.global_node_count = fixture.node_count_global();

    //----------------------------------
    // Create the local sparse matrix graph and element-to-graph map
    // from the element->to->node identifier array.
    // The graph only has rows for the owned nodes.


    typename NodeNodeGraphType::Times graph_times;

    const NodeNodeGraphType
      mesh_to_graph( fixture.elem_node() , fixture.node_count_owned(), graph_times );

    perf.map_ratio          = maximum(comm, graph_times.ratio);
    perf.fill_node_set      = maximum(comm, graph_times.fill_node_set);
    perf.scan_node_count    = maximum(comm, graph_times.scan_node_count);
    perf.fill_graph_entries = maximum(comm, graph_times.fill_graph_entries);
    perf.sort_graph_entries = maximum(comm, graph_times.sort_graph_entries);
    perf.fill_element_graph = maximum(comm, graph_times.fill_element_graph);

    wall_clock.reset();
    // Create the local sparse matrix from the graph:

    LocalMatrixType jacobian( "jacobian" , mesh_to_graph.graph );

    Device::fence();

    perf.create_sparse_matrix = maximum( comm , wall_clock.seconds() );

    //----------------------------------

    if ( print_flag ) {
      const unsigned nrow = jacobian.numRows();
      std::cout << "JacobianGraph[ "
                << jacobian.numRows() << " x " << jacobian.numCols()
                << " ] {" << std::endl ;
      for ( unsigned irow = 0 ; irow < nrow ; ++irow ) {
        std::cout << "  row[" << irow << "]{" ;
        const unsigned entry_end = jacobian.graph.row_map(irow+1);
        for ( unsigned entry = jacobian.graph.row_map(irow) ; entry < entry_end ; ++entry ) {
          std::cout << " " << jacobian.graph.entries(entry);
        }
        std::cout << " }" << std::endl ;
      }
      std::cout << "}" << std::endl ;

      std::cout << "ElemGraph {" << std::endl ;
      for ( unsigned ielem = 0 ; ielem < mesh_to_graph.elem_graph.dimension_0() ; ++ielem ) {
        std::cout << "  elem[" << ielem << "]{" ;
        for ( unsigned irow = 0 ; irow < mesh_to_graph.elem_graph.dimension_1() ; ++irow ) {
          std::cout << " {" ;
          for ( unsigned icol = 0 ; icol < mesh_to_graph.elem_graph.dimension_2() ; ++icol ) {
            std::cout << " " << mesh_to_graph.elem_graph(ielem,irow,icol);
          }
          std::cout << " }" ;
        }
        std::cout << " }" << std::endl ;
      }
      std::cout << "}" << std::endl ;
    }

    //----------------------------------

    // Allocate solution vector for each node in the mesh and residual vector for each owned node
    // We need dual vectors for Tpetra!!
    const LocalDualVectorType k_nodal_solution( "nodal_solution" , fixture.node_count(),1 );
    const LocalDualVectorType k_nodal_residual( "nodal_residual" , fixture.node_count_owned(),1 );
    const LocalDualVectorType k_nodal_delta(    "nodal_delta" ,    fixture.node_count_owned(),1 );
    const LocalVectorType nodal_solution = Kokkos::subview<LocalVectorType>(k_nodal_solution.d_view,Kokkos::ALL(),0);
    const LocalVectorType nodal_residual = Kokkos::subview<LocalVectorType>(k_nodal_residual.d_view,Kokkos::ALL(),0);
    const LocalVectorType nodal_delta = Kokkos::subview<LocalVectorType>(k_nodal_delta.d_view,Kokkos::ALL(),0);

    // Create element computation functor
    const ElementComputationType elemcomp(
      use_atomic ? ElementComputationType( fixture , manufactured_solution.K , nodal_solution ,
                                           mesh_to_graph.elem_graph , jacobian , nodal_residual )
                 : ElementComputationType( fixture , manufactured_solution.K , nodal_solution ) );

    const NodeElemGatherFillType gatherfill(
      use_atomic ? NodeElemGatherFillType()
                 : NodeElemGatherFillType( fixture.elem_node() ,
                                           mesh_to_graph.elem_graph ,
                                           nodal_residual ,
                                           jacobian ,
                                           elemcomp.elem_residuals ,
                                           elemcomp.elem_jacobians ) );

    // Create boundary condition functor
    const DirichletComputationType dirichlet(
      fixture , nodal_solution , jacobian , nodal_residual ,
      2 /* apply at 'z' ends */ ,
      manufactured_solution.T_zmin ,
      manufactured_solution.T_zmax );


    const ::Teuchos::ParameterList params();

    // Create Distributed Objects

    // Create Node
    RCP<NodeType> node = rcp (new NodeType());

    // Create Maps
    ::Kokkos::View<int*,Device> lid_to_gid_row("lig_to_gid",jacobian.numRows());
    for(int i=0;i<jacobian.numRows();i++) lid_to_gid_row(i) = fixture.node_global_index(i);

    pMapType RowMap = ::Teuchos::rcp (new MapType (fixture.node_count_global(),
        ::Teuchos::arrayView(lid_to_gid_row.ptr_on_device(),lid_to_gid_row.dimension_0()),
        0, comm, node));

    ::Kokkos::View<int*,Device> lid_to_gid("lig_to_gid",jacobian.numCols());
    for(int i=0;i<jacobian.numCols();i++) lid_to_gid(i) = fixture.node_global_index(i);

    pMapType ColMap = ::Teuchos::rcp (new MapType ( (fixture.node_count_global()),
        ::Teuchos::arrayView(lid_to_gid.ptr_on_device(),lid_to_gid.dimension_0()),
         0,comm, node) );

    // Create Teptra Matrix: this uses the already allocated matrix data
    GlobalMatrixType g_jacobian(RowMap,ColMap,jacobian);

    // Create Teptra Vectors: this uses the already allocated vector data
    GlobalVectorType g_nodal_solution(ColMap,k_nodal_solution);
    GlobalVectorType g_nodal_residual(RowMap,k_nodal_residual);
    GlobalVectorType g_nodal_delta(RowMap,k_nodal_delta);

    // Create a subview of just the owned data of the solution vector
    GlobalVectorType g_nodal_solution_no_overlap(RowMap,Kokkos::subview<LocalDualVectorType>(k_nodal_solution,std::pair<unsigned,unsigned>(0,k_nodal_delta.dimension_0()),Kokkos::ALL()));
    //::Teuchos::RCP<GlobalVectorType> temp = g_nodal_solution.offsetViewNonConst (RowMap, 0);
    //GlobalVectorType g_nodal_solution_no_overlap (*temp); // shallow copy

    typedef ::Tpetra::Import<typename GlobalVectorType::local_ordinal_type,
      typename GlobalVectorType::global_ordinal_type,
      typename GlobalVectorType::node_type> import_type;
    import_type import (RowMap, ColMap);

    //----------------------------------
    // Nonlinear Newton iteration:

    double residual_norm_init = 0 ;

    for ( perf.newton_iter_count = 0 ;
          perf.newton_iter_count < newton_iteration_limit ;
          ++perf.newton_iter_count ) {

      //--------------------------------

      //comm_nodal_import( nodal_solution );
      g_nodal_solution.doImport (g_nodal_solution_no_overlap, import, Tpetra::REPLACE);

      //--------------------------------
      // Element contributions to residual and jacobian

      wall_clock.reset();

      Kokkos::deep_copy( nodal_residual , double(0) );
      Kokkos::deep_copy( jacobian.values , double(0) );

      elemcomp.apply();

      if ( ! use_atomic ) {
        gatherfill.apply();
      }

      Device::fence();
      perf.fill_time = maximum( comm , wall_clock.seconds() );

      //--------------------------------
      // Apply boundary conditions

      wall_clock.reset();

      dirichlet.apply();

      Device::fence();
      perf.bc_time = maximum( comm , wall_clock.seconds() );

      //--------------------------------
      // Evaluate convergence

      const double residual_norm =
          g_nodal_residual.norm2();

      perf.newton_residual = residual_norm ;

      if ( 0 == perf.newton_iter_count ) { residual_norm_init = residual_norm ; }

      if ( residual_norm < residual_norm_init * newton_iteration_tolerance ) { break ; }

      //--------------------------------
      // Solve for nonlinear update

      result_struct cgsolve = cg_solve(
          Teuchos::rcpFromRef(g_jacobian),
          Teuchos::rcpFromRef(g_nodal_residual),
          Teuchos::rcpFromRef(g_nodal_delta),
          cg_iteration_limit , cg_iteration_tolerance);

      // Update solution vector

      g_nodal_solution_no_overlap.update(-1.0,g_nodal_delta,1.0);
      perf.cg_iter_count += cgsolve.iteration ;
      perf.cg_time       += cgsolve.iter_time ;

      //--------------------------------

      if ( print_flag ) {
        const double delta_norm =
            g_nodal_delta.norm2();

        std::cout << "Newton iteration[" << perf.newton_iter_count << "]"
                  << " residual[" << perf.newton_residual << "]"
                  << " update[" << delta_norm << "]"
                  << " cg_iteration[" << cgsolve.iteration << "]"
                  << " cg_residual[" << cgsolve.norm_res << "]"
                  << std::endl ;

        const unsigned nrow = jacobian.numRows();

        std::cout << "Residual {" ;
        for ( unsigned irow = 0 ; irow < nrow ; ++irow ) {
          std::cout << " " << nodal_residual(irow);
        }
        std::cout << " }" << std::endl ;

        std::cout << "Delta {" ;
        for ( unsigned irow = 0 ; irow < nrow ; ++irow ) {
          std::cout << " " << nodal_delta(irow);
        }
        std::cout << " }" << std::endl ;

        std::cout << "Solution {" ;
        for ( unsigned irow = 0 ; irow < nrow ; ++irow ) {
          std::cout << " " << nodal_solution(irow);
        }
        std::cout << " }" << std::endl ;

        std::cout << "Jacobian[ "
                  << jacobian.numRows() << " x " << jacobian.numCols()
                  << " ] {" << std::endl ;
        for ( unsigned irow = 0 ; irow < nrow ; ++irow ) {
          std::cout << "  {" ;
          const unsigned entry_end = jacobian.graph.row_map(irow+1);
          for ( unsigned entry = jacobian.graph.row_map(irow) ; entry < entry_end ; ++entry ) {
            std::cout << " (" << jacobian.graph.entries(entry)
                      << "," << jacobian.values(entry)
                      << ")" ;
          }
          std::cout << " }" << std::endl ;
        }
        std::cout << "}" << std::endl ;
      }

      //--------------------------------
    }

    // Evaluate solution error

    if ( 0 == itrial ) {
      const typename FixtureType::node_coord_type::HostMirror
        h_node_coord = Kokkos::create_mirror_view( fixture.node_coord() );

      const typename LocalVectorType::HostMirror
        h_nodal_solution = Kokkos::create_mirror_view( nodal_solution );

      Kokkos::deep_copy( h_node_coord , fixture.node_coord() );
      Kokkos::deep_copy( h_nodal_solution , nodal_solution );

      double error_max = 0 ;
      for ( unsigned inode = 0 ; inode < fixture.node_count_owned() ; ++inode ) {
        const double answer = manufactured_solution( h_node_coord( inode , 2 ) );
        const double error = ( h_nodal_solution(inode) - answer ) / answer ;
        if ( error_max < fabs( error ) ) { error_max = fabs( error ); }
      }

      perf.error_max = std::sqrt( Kokkos::Example::all_reduce_max( error_max , comm ) );

      perf_stats = perf ;
    }
    else {
      perf_stats.fill_node_set = std::min( perf_stats.fill_node_set , perf.fill_node_set );
      perf_stats.scan_node_count = std::min( perf_stats.scan_node_count , perf.scan_node_count );
      perf_stats.fill_graph_entries = std::min( perf_stats.fill_graph_entries , perf.fill_graph_entries );
      perf_stats.sort_graph_entries = std::min( perf_stats.sort_graph_entries , perf.sort_graph_entries );
      perf_stats.fill_element_graph = std::min( perf_stats.fill_element_graph , perf.fill_element_graph );
      perf_stats.create_sparse_matrix = std::min( perf_stats.create_sparse_matrix , perf.create_sparse_matrix );
      perf_stats.fill_time = std::min( perf_stats.fill_time , perf.fill_time );
      perf_stats.bc_time = std::min( perf_stats.bc_time , perf.bc_time );
      perf_stats.cg_time = std::min( perf_stats.cg_time , perf.cg_time );
    }
  }

  return perf_stats ;
}

} /* namespace FENL */
} /* namespace Example */
} /* namespace Kokkos */

#endif /* #ifndef KOKKOS_EXAMPLE_FENL_IMPL_HPP */

