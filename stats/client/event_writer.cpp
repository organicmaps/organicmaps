#include "event_writer.hpp"

#include "../../base/string_format.hpp"

#include "../../std/ctime.hpp"

namespace stats
{

EventWriter::EventWriter(string const & uniqueClientId, string const & dbPath)
  : /*m_cnt(0), m_db(0), */m_path(dbPath)/*, m_uid(0)*/
{
}

bool EventWriter::Store(const Event & e)
{
  // @todo add impl
  return false;
}

bool EventWriter::OpenDb(string const & path)
{
  // @todo add impl
  return false;
}

} // namespace stats
