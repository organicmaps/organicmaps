#pragma once

#include "generator/osm_id.hpp"

#include "std/fstream.hpp"
#include "std/mutex.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace generator
{
class SyncOfstream
{
public:
  void Open(string const & fullPath);
  bool IsOpened();
  void Write(uint32_t featureId, vector<osm::Id> const & osmIds);

private:
  ofstream m_stream;
  mutex m_mutex;
};
}  // namespace generator
