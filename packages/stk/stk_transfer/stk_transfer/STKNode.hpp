
#ifndef  STK_STKNODE_HPP
#define  STK_STKNODE_HPP

#include <boost/shared_ptr.hpp>

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


template <unsigned DIM> class STKNode {
public :
  typedef mesh:: Entity                                      Entity;
  typedef std::vector<Entity>                                EntityVec;
  typedef mesh:: EntityKey                                   EntityKey;
  typedef std::set   <EntityKey>                             EntityKeySet;
  typedef search::ident::IdentProc<EntityKey, unsigned>      EntityProc;
  typedef std::vector<EntityProc>                            EntityProcVec;

  typedef search::box::SphereBoundingBox<EntityProc,float,DIM> BoundingBox;


  enum {Dimension = DIM};

  STKNode(const EntityVec                     &ent,
          const mesh::FieldBase               &coord,
          const std::vector<mesh::FieldBase*> &val,
          const double                         initial_radius);
  ~STKNode();

  // Needed for STK Transfer
  ParallelMachine comm() const {return m_comm;}

  void bounding_boxes (std::vector<BoundingBox> &v) const;

  void copy_entities(const EntityProcVec    &entities_to_copy,
                     const std::string         &transfer_name);
  
  void update_values();

  // Needed for LinearInterpoate and FEInterpolation
  const double *coord(const EntityKey k) const;
  const double *value(const EntityKey k, const unsigned i=0) const;
        double *value(const EntityKey k, const unsigned i=0);
  unsigned      value_size(const EntityKey e, const unsigned i=0) const;
  unsigned      num_values() const;

  struct Record { virtual ~Record(){} };
  template <class T> T* database(const EntityKey k) {
    typename RecordMap::iterator i = m_record_map.find(k);
    if (i == m_record_map.end()) {
      RecordPtr record(new T());
      i = m_record_map.insert(RecordMap::value_type(k,record)); 
    }
    T *record = dynamic_cast<T*>(i->second.get());
    ThrowRequireMsg (record,__FILE__<<":"<<__LINE__<<" Dynamic Cast failed in STKNode::record ");
    return record;
  }

private :
  STKNode (); 
  STKNode(const STKNode &M);
  STKNode &operator=(const STKNode&);

  mesh::BulkData                        &m_bulk_data;
  bool                               m_mesh_modified;
  const ParallelMachine                       m_comm;
  const double                          m_sphere_rad;
  const EntityKeySet                   m_entity_keys;
  const mesh::FieldBase         &m_coordinates_field;
  const std::vector<mesh::FieldBase*> m_values_field;

  mesh::Ghosting       *m_transfer_entity_ghosting;
  mesh::EntityProcVec   m_entities_currently_ghosted;
  
  typedef  boost::shared_ptr<Record>         RecordPtr;
  typedef  std::map<EntityKey,RecordPtr>     RecordMap;
  RecordMap                             m_record_map;

  Entity entity(const EntityKey k) const;
  static EntityKeySet entity_keys (const mesh::BulkData &bulk_data, const EntityVec &ent);
};


template<unsigned DIM> typename STKNode<DIM>::EntityKeySet STKNode<DIM>::entity_keys (
  const mesh::BulkData  &bulk_data,
  const       EntityVec &entities){
  EntityKeySet entity_keys;
  for (EntityVec::const_iterator e=entities.begin(); e!=entities.end(); ++e) {
    const mesh::EntityKey k = bulk_data.entity_key(*e);
    entity_keys.insert(k);
  }
  return entity_keys;
}

template<unsigned DIM> STKNode<DIM>::STKNode(
          const            EntityVec          &entities,
          const   mesh::FieldBase             &coord,
          const std::vector<mesh::FieldBase*> &val,
          const double                         initial_radius) :
    m_bulk_data         (coord.get_mesh()),
    m_mesh_modified     (false),
    m_comm              (m_bulk_data.parallel()),
    m_sphere_rad        (initial_radius),
    m_entity_keys       (entity_keys(m_bulk_data, entities)), 
    m_coordinates_field (coord), 
    m_values_field      (val),
    m_entities_currently_ghosted() {
  const std::string name = "Transfer Ghosting";
  m_bulk_data.modification_begin();
  m_transfer_entity_ghosting = &m_bulk_data.create_ghosting(name);
  m_bulk_data.modification_end();
}

template<unsigned DIM> STKNode<DIM>::~STKNode(){}

template<unsigned DIM> void STKNode<DIM>::bounding_boxes (std::vector<BoundingBox> &v) const {

  typedef typename BoundingBox::Data Data;
  typedef typename BoundingBox::Key  Key;
  const Data r=m_sphere_rad;

  v.clear();

  for (typename EntityKeySet::const_iterator k=m_entity_keys.begin(); k!=m_entity_keys.end(); ++k) {
    const EntityKey Id = *k; 
    Data center[Dimension];
    const double *c = coord(Id);
    for (unsigned j=0; j<Dimension; ++j) center[j] = c[j];
    const Key key(Id, parallel_machine_rank(m_comm));
    BoundingBox B(center, r, key);
    v.push_back(B);
  }
}

template<unsigned NUM> void STKNode<NUM>::copy_entities(
                     const EntityProcVec  &keys_to_copy,
                     const std::string    &transfer_name) {

  m_bulk_data.modification_begin();
  {
    mesh::EntityProcVec new_entities_to_copy(keys_to_copy.size());
    for (size_t i=0; i<keys_to_copy.size(); ++i) {
      // convert from EntityProc based on EntityKey to EntityProc based on raw Entity.
      const EntityProc key_proc = keys_to_copy[i];
      const EntityKey       key = key_proc.ident;
      const unsigned       proc = key_proc.proc;
      const Entity            e = entity(key);
      const mesh::EntityProc ep( e, proc);
      new_entities_to_copy[i] = ep;
    } 
    m_entities_currently_ghosted.insert(m_entities_currently_ghosted.end(), 
                                        new_entities_to_copy.begin(), 
                                        new_entities_to_copy.end());

    std::sort(m_entities_currently_ghosted.begin(), m_entities_currently_ghosted.end());
    mesh::EntityProcVec::iterator del = std::unique(m_entities_currently_ghosted.begin(), m_entities_currently_ghosted.end());
    m_entities_currently_ghosted.resize(std::distance(m_entities_currently_ghosted.begin(), del));
  }
  {
    m_bulk_data.change_ghosting(*m_transfer_entity_ghosting,
                                m_entities_currently_ghosted);

    std::vector<mesh::EntityKey> receive;
    std::vector<mesh::EntityProc> send;
    m_transfer_entity_ghosting->receive_list( receive );
    m_transfer_entity_ghosting->send_list( send );
  }
  m_mesh_modified = true;
  m_bulk_data.modification_end();
}

template<unsigned DIM> void STKNode<DIM>::update_values () {
  std::vector<const mesh::FieldBase *> fields(m_values_field.begin(), m_values_field.end());
  if (m_mesh_modified) {
    // Copy coordinates to the newly ghosted nodes
    m_mesh_modified = false;
    fields.push_back(&m_coordinates_field);
  }
  mesh::communicate_field_data( *m_transfer_entity_ghosting , fields);
  mesh::copy_owned_to_shared  (  m_bulk_data, fields );
}
  
template<unsigned DIM> const double *STKNode<DIM>::coord(const EntityKey k) const {
  const mesh::Entity e = entity(k);
  const double *c = static_cast<const double*>(m_bulk_data.field_data(m_coordinates_field, e));
  return  c;
}

template<unsigned DIM> unsigned  STKNode<DIM>::num_values() const {
 const unsigned s = m_values_field.size();
 return s;
}

template<unsigned DIM> unsigned  STKNode<DIM>::value_size(const EntityKey k, const unsigned i) const {
  const mesh::Entity         e = entity(k);
  const mesh::FieldBase &field = *m_values_field[i];
  const mesh::Bucket    &bucket= m_bulk_data.bucket(e);

  const unsigned bytes = m_bulk_data.field_data_size_per_entity(field, bucket);
  const unsigned bytes_per_entry = field.data_traits().size_of;
  const unsigned num_entry = bytes/bytes_per_entry;

  ThrowRequireMsg (bytes == num_entry * bytes_per_entry,    
    __FILE__<<":"<<__LINE__<<" Error:" <<"  bytes:" <<bytes<<"  num_entry:" <<num_entry
         <<"  bytes_per_entry:" <<bytes_per_entry);
  return  num_entry;
}

template<unsigned DIM> const double *STKNode<DIM>::value(const EntityKey k, const unsigned i) const {
  const mesh::Entity  e = entity(k);
  mesh::FieldBase *val=m_values_field[i];
  const double *value = static_cast<const double*>(m_bulk_data.field_data(*val, e));
  return  value;
}

template<unsigned DIM> double *STKNode<DIM>::value(const EntityKey k, const unsigned i) {
  const mesh::Entity e = entity(k);
  mesh::FieldBase *val=m_values_field[i];
  double *value = static_cast<double*>(m_bulk_data.field_data(*val, e));
  return  value;
}

template<unsigned DIM> mesh::Entity STKNode<DIM>::entity(const mesh::EntityKey k) const {
  const mesh::Entity  e = m_bulk_data.get_entity(k);
  return e;
}


}
}
#endif
