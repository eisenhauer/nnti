#include <limits>

#include <boost/shared_ptr.hpp>

#include <Intrepid_FieldContainer.hpp>
#include <Intrepid_Utils.hpp>
#include <Intrepid_FunctionSpaceTools.hpp>
#include <Intrepid_CellTools.hpp>
#include <Intrepid_Basis.hpp>

#include <stk_mesh/base/Entity.hpp>
#include <stk_mesh/base/EntityKey.hpp>
#include <stk_mesh/base/FieldBase.hpp>
#include <stk_mesh/base/MetaData.hpp>
#include <stk_mesh/base/BulkData.hpp>
#include <stk_mesh/base/FieldParallel.hpp>

#include <stk_util/environment/ReportHandler.hpp>
#include <stk_search/BoundingBox.hpp>
#include <stk_search/IdentProc.hpp>

namespace stk {
namespace transfer {


namespace STKElemUtil {

typedef Intrepid::FieldContainer<double>   MDArray;
typedef Intrepid::FieldContainer<unsigned> MDArrayUInt;

typedef Intrepid::Basis<double, MDArray> BasisType;
typedef boost::shared_ptr<BasisType>     BasisTypeRCP;
typedef std::map<unsigned,BasisTypeRCP>  BasisTable;

unsigned parametric(std::vector<double> &para_coords,
                    const double *to,
                    const mesh::Entity element,
                    const mesh::FieldBase &coords_field,
                    const mesh::BulkData& bulkData);

void parametric(std::vector<std::vector<double> > &val,
          const std::vector<double>               &para_coords,
          const mesh::Entity                       element,
          const std::vector<mesh::FieldBase*>     &values_field,
          const mesh::BulkData&                   bulkData);

BasisTable setupBasisTable();

inline
const BasisTypeRCP getBasis(const shards::CellTopology& topo)
{
  static const BasisTable basisTable(setupBasisTable());

  const unsigned key = topo.getKey();
  BasisTable::const_iterator b = basisTable.find(key);
  ThrowRequireMsg( (b != basisTable.end()), "No basis available for this topology");

  const BasisTypeRCP basis = b->second;
  return basis;
}

inline
void fill_ref_vals(MDArray &refVals, const MDArray &refPoints, const shards::CellTopology &topo)
{
  const BasisTypeRCP basis = getBasis(topo);
  basis->getValues(refVals, refPoints, Intrepid::OPERATOR_VALUE);
}

} // namespace STKElemUtil

class STKElem {
public :
  typedef mesh:: Entity                               Entity;
  typedef std::vector<Entity>                         EntityVec;
  typedef mesh:: EntityKey                            EntityKey;
  typedef std::set<EntityKey>                         EntityKeySet;
  typedef search::IdentProc<EntityKey, unsigned>      EntityProc;
  typedef std::vector<EntityProc>                     EntityProcVec;

  typedef search::Point<float> Point;
  typedef search::Box<float>   Box;
  typedef std::pair<Box,EntityProc> BoundingBox;

  STKElem(const EntityVec                     &ent,
          const mesh::FieldBase               &coord,
          const std::vector<mesh::FieldBase*> &val);

  // Needed for STK Transfer
  ParallelMachine comm() const {return m_comm;}

  void bounding_boxes (std::vector< std::pair<Box,EntityProc> > &v) const;

  void copy_entities(const EntityProcVec    &entities_to_copy,
                     const std::string         &transfer_name);

  void update_values();

  // Needed for Interpolation

  unsigned      value_size(const EntityKey e, const unsigned i=0) const;
  unsigned      num_values() const { return m_values_field.size(); }
  double parametric_coord(std::vector<double> &coords,
                          const double *to,
                          const EntityKey k ) const;

  void eval_parametric   (std::vector<std::vector<double> > &val,
                    const std::vector<double> &coords,
                    const EntityKey k) const
  {
    const mesh::Entity element = entity(k);
    STKElemUtil::parametric(val, coords, element, m_values_field, m_bulk_data);
  }

private :
  STKElem ();
  STKElem(const STKElem &M);
  STKElem &operator=(const STKElem&);

  mesh::BulkData                        &m_bulk_data;
  bool                               m_mesh_modified;
  const ParallelMachine                       m_comm;
  const EntityKeySet                   m_entity_keys;
  const mesh::FieldBase         &m_coordinates_field;
  const std::vector<mesh::FieldBase*> m_values_field;

  mesh::Ghosting       *m_transfer_entity_ghosting;
  mesh::EntityProcVec   m_entities_currently_ghosted;

  unsigned m_spatial_dimension;

  Entity entity(const EntityKey k) const
  {
    return m_bulk_data.get_entity(k);
  }

  static EntityKeySet entity_keys (const mesh::BulkData &bulk_data, const EntityVec &ent);
  void elem_coord_limits(Point &min_corner, Point &max_corner, const EntityKey k) const;
};

} // namespace transfer
} // namespace stk
