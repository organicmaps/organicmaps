#pragma once

#include "base/exception.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace osm
{
class Id;
}

namespace generator
{
DECLARE_EXCEPTION(DBNotFound, RootException);

class UGCDB
{
public:
  UGCDB(std::string const & path);
  bool Get(osm::Id const & id, std::vector<uint8_t> & blob);
};
}  // namespace generator
