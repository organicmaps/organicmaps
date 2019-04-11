#pragma once

#include "generator/ugc_db.hpp"

#include "ugc/types.hpp"

#include "indexer/feature_data.hpp"

#include "base/geo_object_id.hpp"

#include <string>

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

bool GetUgcForFeature(base::GeoObjectId const & osmId, feature::TypesHolder const & th,
                      UGCTranslator & translator, ugc::UGC & result);
}  // namespace generator
