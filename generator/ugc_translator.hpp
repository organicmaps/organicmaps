#pragma once

#include "ugc_db.hpp"

#include "ugc/types.hpp"

#include "base/geo_object_id.hpp"

namespace generator
{
class UGCTranslator
{
public:
  UGCTranslator();
  UGCTranslator(std::string const & dbFilename);

  bool TranslateUGC(base::GeoObjectId const & id, ugc::UGC & ugc);

  // For testing only
  void CreateDb(std::string const & data);

private:
  UGCDB m_db;
};
}  // namespace generator
