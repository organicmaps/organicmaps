#include "stats_writer.hpp"

#include "../../base/string_format.hpp"

#include "../../std/ctime.hpp"

namespace stats
{

StatsWriter::StatsWriter(string const & uniqueClientId, string const & dbPath)
  : m_cnt(0), m_db(0), m_path(dbPath),
    m_uid(0)
{
}

bool StatsWriter::Store(const Event & e)
{
  // @todo add impl
  return false;
}

bool StatsWriter::OpenDb(string const & path)
{
  // @todo add impl
  return false;
}

} // namespace stats
