#pragma once

#include <string>
#include <vector>
#include <leveldb/db.h>

namespace stats
{

template <class P>
class LevelDBReader
{
public:
  LevelDBReader(string const & db_path) : m_db(NULL), m_it(NULL), m_path(db_path)
  {
  }

  bool ReadNext(P * proto)
  {
    if ((m_it == NULL && !Open()) || !m_it->Valid())
      return false;

    string const s = m_it->value().ToString();
    m_it->Next();

    return proto->ParseFromString(s);
  }

  bool Open()
  {
    leveldb::Options options;
    leveldb::Status status = leveldb::DB::Open(options, m_path, &m_db);

    if (!status.ok())
      return false;

    m_it = m_db->NewIterator(leveldb::ReadOptions());
    m_it->SeekToFirst();
    if (!m_it->status().ok())
    {
      delete m_it;
      m_it = NULL;
    }

    return m_it->status().ok();
  }

  ~LevelDBReader()
  {
    delete m_it;
    delete m_db;
  }

 private:
  leveldb::DB * m_db;
  leveldb::Iterator * m_it;
  string const m_path;
};

template <class P>
vector<P> ReadAllFromLevelDB(string const & db_path)
{
  vector<P> res;

  LevelDBReader<P> reader(db_path);

  P proto;
  while (reader.ReadNext(&proto))
  {
    res.push_back(proto);
  }

  return res;
}

}  // namespace stats
