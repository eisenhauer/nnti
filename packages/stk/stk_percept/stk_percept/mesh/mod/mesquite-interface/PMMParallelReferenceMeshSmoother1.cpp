#if !defined(__IBMCPP__)
#ifdef STK_BUILT_IN_SIERRA

#include <stk_percept/mesh/mod/mesquite-interface/PMMParallelReferenceMeshSmoother1.hpp>
#include <stk_percept/mesh/mod/mesquite-interface/PerceptMesquiteMesh.hpp>
#include <stk_percept/mesh/mod/mesquite-interface/JacobianUtil.hpp>
#include <stk_percept/math/Math.hpp>

#include <stk_mesh/base/FieldParallel.hpp>
#include <stdio.h>

#include "mpi.h"

#define DEBUG_PRINT 0
#define PRINT(a) do { if (DEBUG_PRINT) std::cout << a << std::endl; } while(0)
#define PRINT_1(a) do { std::cout << a << std::endl; } while(0)

namespace MESQUITE_NS {

  extern int get_parallel_rank();
}

namespace stk {
  namespace percept {

    using namespace Mesquite;
    const bool do_tot_test = false;
    bool do_print_elem_val = false;

    bool PMMParallelReferenceMeshSmoother1::check_convergence()
    {
      if (m_stage == 0 && (m_dnew == 0.0 || m_total_metric == 0.0))
        {
          return true; // for untangle
        }
      if (m_num_invalid == 0 && m_dnew < gradNorm*gradNorm*m_d0 && m_dmax < gradNorm)
        {
          return true;
        }      
      return false;
    }

    double PMMParallelReferenceMeshSmoother1::metric(stk::mesh::Entity& element, bool& valid)
    {
      return m_metric->metric(element,valid);
    }

    /// fills cg_g_field with f'(x)
    void PMMParallelReferenceMeshSmoother1::get_gradient( Mesh* mesh, MeshDomain *domain)
    {
      PerceptMesquiteMesh *pmm = dynamic_cast<PerceptMesquiteMesh *>(mesh);
      PerceptMesh *eMesh = pmm->getPerceptMesh();
      stk::mesh::FieldBase *coord_field = eMesh->get_coordinates_field();
      stk::mesh::FieldBase *coord_field_current   = coord_field;
      stk::mesh::FieldBase *cg_g_field    = eMesh->get_field("cg_g");

      stk::mesh::Selector on_locally_owned_part =  ( eMesh->get_fem_meta_data()->locally_owned_part() );
      stk::mesh::Selector on_globally_shared_part =  ( eMesh->get_fem_meta_data()->globally_shared_part() );
      int spatialDim = eMesh->get_spatial_dim();
      bool valid=true;

      // g=0
      eMesh->nodal_field_set_value(cg_g_field, 0.0);

      //if (m_iter == 0) 
      m_scale = 1.e-10;

      // element loop: compute deltas; all elements (even ghosts) contribute to the non-ghosted nodes
      {
        const std::vector<stk::mesh::Bucket*> & buckets = eMesh->get_bulk_data()->buckets( eMesh->element_rank() );

        for ( std::vector<stk::mesh::Bucket*>::const_iterator k = buckets.begin() ; k != buckets.end() ; ++k )
          {
            //if (on_locally_owned_part(**k))  
            // loop over all elements
              {
                stk::mesh::Bucket & bucket = **k ;
                const unsigned num_elements_in_bucket = bucket.size();

                for (unsigned i_element = 0; i_element < num_elements_in_bucket; i_element++)
                  {
                    stk::mesh::Entity& element = bucket[i_element];

                    const mesh::PairIterRelation elem_nodes = element.relations( stk::mesh::fem::FEMMetaData::NODE_RANK );
                    unsigned num_node = elem_nodes.size();

                    double edge_length_ave = m_eMesh->edge_length_ave(element);

                    double metric_0 = metric(element, valid);

                    for (unsigned inode=0; inode < num_node; inode++)
                      {
                        mesh::Entity & node = * elem_nodes[ inode ].entity();

                        bool isGhostNode = !(on_locally_owned_part(node) || on_globally_shared_part(node));
                        bool fixed = pmm->get_fixed_flag(&node);
                        if (fixed || isGhostNode)
                          continue;

                        double *coord_current = PerceptMesh::field_data(coord_field_current, node);
                        double *cg_g = PerceptMesh::field_data(cg_g_field, node);
                        
                        double eps = 1.e-6;
                        double eps1 = eps*edge_length_ave;

                        for (int idim=0; idim < spatialDim; idim++)
                          {
                            coord_current[idim] += eps1;
                            double mp = metric(element, valid);
                            coord_current[idim] -= 2.0*eps1;
                            double mm = metric(element, valid);
                            coord_current[idim] += eps1;
                            double dd = (mp - mm)/(2*eps1);

                            m_scale = std::max(m_scale, std::abs(dd)/edge_length_ave);

                            cg_g[idim] += dd;
                            
                            // FIXME 
                            if (0)
                            {
                              coord_current[idim] -= dd*eps;
                              double m1=metric(element, valid);
                              coord_current[idim] += dd*eps;
                              if (metric_0 > 1.e-6)
                                {
                                  if (!(m1 < metric_0*(1.0+eps)))
                                    {
                                      PRINT( "bad grad" << " m1-metric_0 = " << (m1-metric_0) );
                                    }
                                  VERIFY_OP_ON(m1, <, metric_0*(1.0+eps), "bad gradient");
                                }
                            }
                          }
                      }
                  }
              }
          }
      }

      //if (m_iter == 0) 
        {
          m_scale = (m_scale < 1.0) ? 1.0 : 1.0/m_scale;
          PRINT("tmp srk m_scale= " << m_scale);
          stk::all_reduce( m_eMesh->get_bulk_data()->parallel() , ReduceMin<1>( & m_scale ) );
        }
    }

    double PMMParallelReferenceMeshSmoother1::run_one_iteration( Mesh* mesh, MeshDomain *domain,
                                                              MsqError& err )
    {
      PerceptMesquiteMesh *pmm = dynamic_cast<PerceptMesquiteMesh *>(mesh);
      PerceptMesh *eMesh = pmm->getPerceptMesh();

      stk::mesh::FieldBase *cg_g_field    = eMesh->get_field("cg_g");
      stk::mesh::FieldBase *cg_r_field    = eMesh->get_field("cg_r");
      stk::mesh::FieldBase *cg_d_field    = eMesh->get_field("cg_d");
      stk::mesh::FieldBase *cg_s_field    = eMesh->get_field("cg_s");

      stk::mesh::Selector on_locally_owned_part =  ( eMesh->get_fem_meta_data()->locally_owned_part() );
      stk::mesh::Selector on_globally_shared_part =  ( eMesh->get_fem_meta_data()->globally_shared_part() );
      bool total_valid=true;

      if (m_iter == 0)
        {
          m_dmax = 0.0;

          get_gradient(mesh, domain);
          /// r = -g
          eMesh->nodal_field_axpby(-1.0, cg_g_field, 0.0, cg_r_field);
          /// s = r  (allows for preconditioning later s = M^-1 r)
          eMesh->copy_field(cg_s_field, cg_r_field);
          /// d = s
          eMesh->copy_field(cg_d_field, cg_s_field);
          /// dnew = r.d
          m_dnew = eMesh->nodal_field_dot(cg_r_field, cg_d_field);
          PRINT("tmp srk m_dnew[0] = " << m_dnew);
          /// d0 = dnew
          m_d0 = m_dnew;
        }

      double metric_check = total_metric(mesh, 0.0, 1.0, total_valid);
      m_total_metric = metric_check;
      if (check_convergence() || metric_check == 0.0)
        {
          PRINT_1( "tmp srk already converged m_dnew= " << m_dnew << " gradNorm= " << gradNorm << " m_d0= " << m_d0 );
          //update_node_positions
          return total_metric(mesh,0.0,1.0, total_valid);
        }

      m_dd = eMesh->nodal_field_dot(cg_d_field, cg_d_field);
      PRINT(" tmp srk m_dd= " << m_dd);

      /// line search
      double alpha = m_scale;
      {
        double metric_1 = total_metric(mesh, 1.e-6, 1.0, total_valid);
        double metric_0 = total_metric(mesh, 0.0, 1.0, total_valid);
        double norm_gradient = eMesh->nodal_field_dot(cg_g_field, cg_g_field);
        PRINT( "tmp srk norm_gradient= " << norm_gradient );
        PRINT( "tmp srk " << " metric_0= " << metric_0 << " metric(1.e-6) = " << metric_1 << " diff= " << metric_1-metric_0 );
        metric_1 = total_metric(mesh, -1.e-6, 1.0, total_valid);
        PRINT( "tmp srk " << " metric_0= " << metric_0 << " metric(-1.e-6)= " << metric_1 << " diff= " << metric_1-metric_0 );
        double metric=0.0;
        //double sigma=0.95;
        double tau = 0.5;
        double c0 = 1.e-4;

        get_gradient(mesh, domain);
        double norm_gradient2 = eMesh->nodal_field_dot(cg_g_field, cg_g_field);

        double armijo_offset_factor = c0*norm_gradient2;
        bool converged = false;
        while (!converged)
          {
            metric = total_metric(mesh, alpha, 1.0, total_valid);
            //converged = (metric > sigma*metric_0) && (alpha > 1.e-16);
            double mfac = alpha*armijo_offset_factor;
            converged = (metric < metric_0 + mfac);
            if (m_untangled) converged = converged && total_valid;
            PRINT(  "tmp srk alpha= " << alpha << " metric_0= " << metric_0 << " metric= " << metric << " diff= " << metric - (metric_0 + mfac) 
                    << " m_untangled = " << m_untangled
                    << " total_valid= " << total_valid );
            if (!converged)
              alpha *= tau;
            if (alpha < std::max(1.e-6*m_scale, 1.e-16))
              break;
          }
        //if (metric > sigma*metric_0)
        if (!converged)
          {
            PRINT_1( "can't reduce metric= " << metric << " metric_0 + armijo_offset " << metric_0+alpha*armijo_offset_factor );
            do_print_elem_val = true;
            metric_1 = total_metric(mesh, 1.e-6, 1.0, total_valid);
            metric_0 = total_metric(mesh, 0.0, 1.0, total_valid);
            do_print_elem_val = false;
            PRINT_1( "tmp srk " << " metric_0= " << metric_0 << " metric(1.e-6) = " << metric_1 << " diff= " << metric_1-metric_0 );
            metric_1 = total_metric(mesh, -1.e-6, 1.0, total_valid);
            PRINT_1( "tmp srk " << " metric_0= " << metric_0 << " metric(-1.e-6)= " << metric_1 << " diff= " << metric_1-metric_0 );

            throw std::runtime_error("can't reduce metric");
          }
        else
          {
            double a1 = alpha/2.;
            double a2 = alpha;
            double f0 = metric_0, f1 = total_metric(mesh, a1, 1.0, total_valid), f2 = total_metric(mesh, a2, 1.0, total_valid);
            double den = 2.*(a2*(-f0 + f1) + a1*(f0 - f2));
            double num = a2*a2*(f1-f0)+a1*a1*(f0-f2);
            if (std::fabs(den) > 1.e-10)
              {
                double alpha_quadratic = num/den;
                if (alpha_quadratic < 2*alpha)
                  {
                    double fm=total_metric(mesh, alpha_quadratic, 1.0, total_valid);
                    //if (fm < f2 && (!m_untangled || total_valid))
                    if (fm < f2)
                      {
                        alpha = alpha_quadratic;
                        PRINT( "tmp srk alpha_quadratic= " << alpha_quadratic << " alpha= " << a2 );
                      }
                  } 
              }
          }
      }

      /// x = x + alpha*d
      update_node_positions(mesh, alpha);
      PRINT( "tmp srk iter= "<< m_iter << " dmax= " << m_dmax << " alpha= " << alpha);

      /// f'(x)
      get_gradient(mesh, domain);

      /// r = -g
      eMesh->nodal_field_axpby(-1.0, cg_g_field, 0.0, cg_r_field);

      /// dold = dnew
      m_dold = m_dnew;

      /// dmid = r.s
      m_dmid = eMesh->nodal_field_dot(cg_r_field, cg_s_field);

      /// s = r
      eMesh->copy_field(cg_s_field, cg_r_field);

      /// dnew = r.s
      m_dnew = eMesh->nodal_field_dot(cg_r_field, cg_s_field);
      double f_cur=total_metric(mesh, 0.0, 1.0, total_valid);
      PRINT("tmp srk m_dnew[n] = " << m_dnew << " f_cur= " << f_cur << " total_valid= " << total_valid);

      double cg_beta = 0.0;
      if (std::fabs(m_dold) < 1.e-12) 
        cg_beta = 0.0;
      else
        cg_beta = (m_dnew - m_dmid) / m_dold;

      PRINT("tmp srk beta = " << cg_beta);

      //int N = num_nodes;
      //if (m_iter == N
      if (cg_beta <= 0.0) 
        {
          /// d = s
          eMesh->copy_field(cg_d_field, cg_s_field);
        }
      else
        {
          /// d = s + beta * d
          eMesh->nodal_field_axpby(1.0, cg_s_field, cg_beta, cg_d_field);
        }

      //MSQ_ERRRTN(err);
      return total_metric(mesh,0.0,1.0, total_valid);
      
    }
    
    void PMMParallelReferenceMeshSmoother1::update_node_positions(Mesh* mesh, double alpha)
    {
      PerceptMesquiteMesh *pmm = dynamic_cast<PerceptMesquiteMesh *>(mesh);
      PerceptMesh *eMesh = pmm->getPerceptMesh();
      stk::mesh::Selector on_locally_owned_part =  ( eMesh->get_fem_meta_data()->locally_owned_part() );
      stk::mesh::Selector on_globally_shared_part =  ( eMesh->get_fem_meta_data()->globally_shared_part() );
      int spatialDim = eMesh->get_spatial_dim();
      stk::mesh::FieldBase *cg_d_field    = eMesh->get_field("cg_d");

      m_dmax = 0.0;
      // node loop: update node positions
      {
        const std::vector<stk::mesh::Bucket*> & buckets = eMesh->get_bulk_data()->buckets( eMesh->node_rank() );
        for ( std::vector<stk::mesh::Bucket*>::const_iterator k = buckets.begin() ; k != buckets.end() ; ++k )
          {
            // update local and globally shared 
            if (on_locally_owned_part(**k) || on_globally_shared_part(**k))
              {
                stk::mesh::Bucket & bucket = **k ;
                const unsigned num_nodes_in_bucket = bucket.size();

                for (unsigned i_node = 0; i_node < num_nodes_in_bucket; i_node++)
                  {
                    stk::mesh::Entity& node = bucket[i_node];
                    bool fixed = pmm->get_fixed_flag(&node);
                    bool isGhostNode = !(on_locally_owned_part(node) || on_globally_shared_part(node));
                    if (fixed || isGhostNode)
                      {
                        continue;
                      }

                    double *coord_current = PerceptMesh::field_data(m_coord_field_current, node);
                    double *cg_d = PerceptMesh::field_data(cg_d_field, node);

                    for (int i=0; i < spatialDim; i++)
                      {
                        double dt = alpha*cg_d[i];
                        m_dmax = std::max(std::fabs(dt), m_dmax);
                        coord_current[i] += dt;  
                      }
                  }
              }
          }
      }
    }

    double PMMParallelReferenceMeshSmoother1::total_metric(Mesh *mesh, double alpha, double multiplicative_edge_scaling, bool& valid)
    {
      PerceptMesquiteMesh *pmm = dynamic_cast<PerceptMesquiteMesh *>(mesh);
      stk::mesh::FieldBase *coord_field = m_eMesh->get_coordinates_field();
      stk::mesh::FieldBase *coord_field_current   = coord_field;
      stk::mesh::FieldBase *coord_field_lagged  = m_eMesh->get_field("coordinates_lagged");

      stk::mesh::FieldBase *cg_d_field    = m_eMesh->get_field("cg_d");

      stk::mesh::Selector on_locally_owned_part =  ( m_eMesh->get_fem_meta_data()->locally_owned_part() );
      stk::mesh::Selector on_globally_shared_part =  ( m_eMesh->get_fem_meta_data()->globally_shared_part() );
      int spatialDim = m_eMesh->get_spatial_dim();

      double mtot = 0.0;

      // cache coordinates
      m_eMesh->copy_field(coord_field_lagged, coord_field_current);

      // node loop
      {
        const std::vector<stk::mesh::Bucket*> & buckets = m_eMesh->get_bulk_data()->buckets( m_eMesh->node_rank() );
        for ( std::vector<stk::mesh::Bucket*>::const_iterator k = buckets.begin() ; k != buckets.end() ; ++k )
          {
            // update local and globally shared 
            if (on_locally_owned_part(**k) || on_globally_shared_part(**k))
              {
                stk::mesh::Bucket & bucket = **k ;
                const unsigned num_nodes_in_bucket = bucket.size();

                for (unsigned i_node = 0; i_node < num_nodes_in_bucket; i_node++)
                  {
                    stk::mesh::Entity& node = bucket[i_node];
                    bool fixed = pmm->get_fixed_flag(&node);
                    if (fixed)
                      {
                        continue;
                      }

                    double *coord_current = PerceptMesh::field_data(coord_field_current, node);
                    double *cg_d = PerceptMesh::field_data(cg_d_field, node);

                    for (int i=0; i < spatialDim; i++)
                      {
                        double dt = alpha * cg_d[i];
                        coord_current[i] += dt;
                      }
                  }
              }
          }
      }

      valid = true;
      // element loop
      {
        const std::vector<stk::mesh::Bucket*> & buckets = m_eMesh->get_bulk_data()->buckets( m_eMesh->element_rank() );

        for ( std::vector<stk::mesh::Bucket*>::const_iterator k = buckets.begin() ; k != buckets.end() ; ++k )
          {
            if (on_locally_owned_part(**k))  
              {
                stk::mesh::Bucket & bucket = **k ;
                const unsigned num_elements_in_bucket = bucket.size();

                for (unsigned i_element = 0; i_element < num_elements_in_bucket; i_element++)
                  {
                    stk::mesh::Entity& element = bucket[i_element];
                    bool local_valid=true;
                    double mm = metric(element, local_valid);
                    valid = valid && local_valid;
                    if (do_print_elem_val) PRINT( "element= " << element.identifier() << " metric= " << mm );
                    mtot += mm;
                  }
              }
          }
      }

      // reset coordinates
      m_eMesh->copy_field(coord_field_current, coord_field_lagged);

      stk::all_reduce( m_eMesh->get_bulk_data()->parallel() , ReduceSum<1>( & mtot ) );
      stk::all_reduce( m_eMesh->get_bulk_data()->parallel() , ReduceMax<1>( & valid ) );

      return mtot;
      
    }
  }
}


#endif
#endif
