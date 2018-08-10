#pragma once

#include "base/exception.hpp"
#include "base/geo_object_id.hpp"
#include "base/macros.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace osm
{
class Id;
}

struct sqlite3;

namespace generator
{
DECLARE_EXCEPTION(DBNotFound, RootException);

class UGCDB
{
public:
  UGCDB(std::string const & path);
  ~UGCDB();

  WARN_UNUSED_RESULT bool Get(base::GeoObjectId const & id, std::vector<uint8_t> & blob);
  WARN_UNUSED_RESULT bool Exec(std::string const & statement);

private:
  bool ValueToBlob(std::string const & src, std::vector<uint8_t> & blob);

  sqlite3 * m_db = nullptr;
};
}  // namespace generator
