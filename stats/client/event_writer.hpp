#pragma once

#include "../../std/string.hpp"

#include "../common/wire.pb.h"

namespace stats
{

class EventWriter
{
public:
  EventWriter(string const & uniqueClientId, string const & dbPath);

  bool Store(Event const & e);

  ~EventWriter() {}

  template<class T>
  bool Write(T const & m)
  {
    // @todo implement
    return false;
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
