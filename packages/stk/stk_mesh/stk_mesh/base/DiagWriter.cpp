/*--------------------------------------------------------------------*/
/*    Copyright 2000 - 2011 Sandia Corporation.                       */
/*    Under the terms of Contract DE-AC04-94AL85000, there is a       */
/*    non-exclusive license for use of this work by or on behalf      */
/*    of the U.S. Government.  Export of this program may require     */
/*    a license from the United States Government.                    */
/*--------------------------------------------------------------------*/

#include <stk_util/util/Bootstrap.hpp>

#include <stk_mesh/base/Types.hpp>
#include <stk_mesh/base/DiagWriter.hpp>
#include <stk_mesh/base/Entity.hpp>
#include <stk_mesh/base/Bucket.hpp>
#include <stk_mesh/base/MetaData.hpp>
#include <stk_mesh/base/BulkData.hpp>

namespace stk {
namespace mesh {

#ifdef STK_MESH_TRACE_ENABLED

namespace {

static stk::diag::Writer* s_diagWriter = NULL;

}

void initDiagWriter(std::ostream& stream)
{
  s_diagWriter = new stk::diag::Writer(stream.rdbuf(),
                                       theDiagWriterParser().parse(std::getenv("MESHLOG")));
}

stk::diag::Writer & theDiagWriter()
{
  ThrowRequireMsg(s_diagWriter != NULL, "Please call initDiagWwriter before theDiagWriter");
  return *s_diagWriter;
}

DiagWriterParser & theDiagWriterParser()
{
  static DiagWriterParser parser;

  return parser;
}

DiagWriterParser::DiagWriterParser()
  : stk::diag::WriterParser()
{
  mask("entity", (unsigned long) (LOG_ENTITY), "Display entity diagnostic information");
  mask("bucket", (unsigned long) (LOG_BUCKET), "Display bucket diagnostic information");
  mask("part",   (unsigned long) (LOG_PART),   "Display bucket diagnostic information");
  mask("field",  (unsigned long) (LOG_FIELD),  "Display bucket diagnostic information");
}

namespace {

void bootstrap()
{
//  diag::registerWriter("meshlog", meshlog, theDiagWriterParser());
}

stk::Bootstrap x(&bootstrap);

} // namespace <unnamed>


#endif

std::string log_to_str(EntityState log)
{
  if (log == 0) {
    return "Not changed";
  }
  else if (log == 1) {
    return "Created";
  }
  else if (log == 2) {
    return "Modified";
  }
  else if (log == 3) {
    return "Marked deleted";
  }
  else {
    ThrowRequireMsg(false, "Unknown log " << log);
  }
  return "";
}

stk::diag::Writer& operator<<(stk::diag::Writer& writer, const Part& part)
{
  return writer << "Part[" << part.name() << ", " << part.mesh_meta_data_ordinal() << "]";
}

stk::diag::Writer& operator<<(stk::diag::Writer& writer, const Entity entity)
{
  // Get bucket of entity
  Bucket* bucket = entity.bucket_ptr();

  EntityKey key = entity.key();
  if (bucket) {
    MetaData& meta_data = MetaData::get(*bucket);
    Part &   owned  = meta_data.locally_owned_part();
    Part &   shared = meta_data.globally_shared_part();
    std::string ownership_info = "unregistered";
    if (bucket->member(owned)) {
      ownership_info = "owned";
    }
    else if (bucket->member(shared)) {
      ownership_info = "shared";
    }
    else if (bucket->size() == 0) {
      ownership_info = "marked deleted";
    }
    else {
      ownership_info = "ghosted";
    }

    writer << "Entity[key:" << print_entity_key(meta_data, key) <<
      ", ownership:" << ownership_info <<
      ", log:" << log_to_str(entity.state()) <<
      ", owner:" << entity.owner_rank();

    writer << ", COMM: ";
    PairIterEntityComm comm_itr = BulkData::get(entity).entity_comm(entity.key());
    for ( ; !comm_itr.empty(); ++comm_itr ) {
      writer << "(ghost:" << comm_itr->ghost_id << ", proc:" << comm_itr->proc << ") ";
    }
  }
  else {
    std::ostringstream out;
    out << "(rank:" << key.rank() << ",id:" << key.id() << ")";

    writer << "Entity[key:" << out.str() << "(NEW ENTITY)";
  }

  return writer << "]";
}

stk::diag::Writer& operator<<(stk::diag::Writer& writer, const EntityKey& key)
{
  return writer << "Entity[rank:" << key.rank() << ", id:" << key.id() << "]";
}

stk::diag::Writer& operator<<(stk::diag::Writer& writer, const EntityProc& entity_proc)
{
  return writer << "EntityProc[entity:" << entity_proc.first << ", proc: " << entity_proc.second << "]";
}

} // namespace mesh
} // namespace stk

int dummy_DiagWriter()
{
  // This function is present just to put a symbol in the object
  // file and eliminate a "empty object file" warning on the mac...
  return 1;
}
