/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/

#include <stk_rebalance_utils/RebalanceUtils.hpp>

#include <stk_util/parallel/ParallelReduce.hpp>
#include <stk_mesh/base/GetEntities.hpp>
#include <stk_mesh/base/Field.hpp>
#include <stk_mesh/base/FieldData.hpp>
#include <stk_mesh/base/Entity.hpp>
#include <stk_mesh/base/BulkData.hpp>


//----------------------------------------------------------------------

double stk::rebalance::check_balance(mesh::BulkData &    bulk_data,
                                      const mesh::Field<double> * load_measure,
                                      const stk::mesh::EntityRank rank,
                                      const mesh::Selector *selector)
{
  const ParallelMachine    &comm = bulk_data.parallel();
  double my_load = 0.0;

  const mesh::MetaData & meta_data = stk::mesh::MetaData::get(bulk_data);

  mesh::EntityVector local_elems;
  if (selector) {
    mesh::get_selected_entities(*selector,
                                bulk_data.buckets(rank),
                                local_elems);
  } else {
    mesh::Selector select_owned( meta_data.locally_owned_part() );
    // Determine imbalance based on current element decomposition
    mesh::get_selected_entities(select_owned,
                                bulk_data.buckets(rank),
                                local_elems);
  }

  for(mesh::EntityVector::iterator elem_it = local_elems.begin(); elem_it != local_elems.end(); ++elem_it)
  {
    if (load_measure) {
      const double * load_val = bulk_data.field_data(*load_measure, *elem_it);
      my_load += *load_val;
    } else {
      my_load += 1;
    }
  }

  double max_load = my_load;
  double tot_load = my_load;

  all_reduce(comm, ReduceMax<1>(&max_load) & ReduceSum<1>(&tot_load));

  const int proc_size = parallel_machine_size(comm);
  const double avg_load = tot_load / proc_size;

  const double imbalance_threshold = max_load / avg_load;

  return imbalance_threshold;
}



bool stk::rebalance::verify_dependent_ownership( const mesh::BulkData & bulk_data,
                                                 const stk::mesh::EntityRank & parent_rank,
                                                 stk::mesh::EntityVector & entities )
{
  bool is_with_elem = true;
  for( size_t i = 0; i < entities.size(); ++i )
  {
    is_with_elem = false;

    stk::mesh::Entity entity = entities[i];
    int owner_proc = bulk_data.parallel_owner_rank(entity);

    stk::mesh::Entity const *rels_j = bulk_data.begin(entity, parent_rank);
    stk::mesh::Entity const *rels_e = bulk_data.end(entity, parent_rank);

    for ( ; rels_j != rels_e; ++rels_j)
    {
      stk::mesh::Entity elem = *rels_j;
      if( owner_proc == bulk_data.parallel_owner_rank(elem) )
      {
        is_with_elem = true;
        break;
      }
    }
    if( !is_with_elem )
      return false;
  }

  return is_with_elem;
}
