#pragma once

#include "../../std/string.hpp"

#include "../common/wire.pb.h"

namespace stats
{

class StatsWriter
{
public:
  StatsWriter(string const & uniqueClientId, string const & dbPath);

  bool Store(Event const & e);

  ~StatsWriter() {}

  template<class T>
  bool Write(T const & m)
  {
    Event e;
    e.MutableExtension(T::event)->CopyFrom(m);
    e.set_userid(m_uid);
    e.set_timestamp(time(NULL));

    return Store(e);
  }

private:
  bool OpenDb(string const & path);

private:
  unsigned int m_cnt;
  void * m_db; // @todo Replace with ours impl
  string m_path;
  unsigned long long m_uid;
};

}  // namespace stats
