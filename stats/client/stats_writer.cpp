#include "stats_writer.hpp"

#include "../../base/string_format.hpp"

#include "../../std/ctime.hpp"

#include <city.h>

namespace stats
{

StatsWriter::StatsWriter(string const & uniqueClientId, string const & dbPath)
  : m_cnt(0), m_db(0), m_path(dbPath),
    m_uid(CityHash64(uniqueClientId.c_str(), uniqueClientId.size()))
{
}

bool StatsWriter::Store(const Event & e)
{
  string buf;
  e.SerializeToString(&buf);

  if (!m_db)
  {
    if(!OpenDb(m_path))
    {
      return false;
    }
  }

  // We can't just make timestamp a key - might have
  // several writes per second.
  string key(strings::ToString(e.timestamp()) + "-" + strings::ToString(m_cnt++));

  leveldb::WriteOptions opt;
  opt.sync = true;  // Synchronous writes.
  return m_db->Put(opt, key, buf).ok();
}

bool StatsWriter::OpenDb(string const & path)
{
  leveldb::Options options;
  options.create_if_missing = true;
  return leveldb::DB::Open(options, path, &m_db).ok();
}

} // namespace stats
