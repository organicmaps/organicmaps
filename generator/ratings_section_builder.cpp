#include "generator/ratings_section_builder.hpp"

#include "generator/ugc_translator.hpp"
#include "generator/utils.hpp"

#include "ugc/binary/index_ugc.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/rank_table.hpp"

#include "base/geo_object_id.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "defines.hpp"

namespace generator
{
bool BuildRatingsMwmSection(std::string const & srcDbFilename, std::string const & mwmFile,
                            std::string const & osmToFeatureFilename)
{
  LOG(LINFO, ("Build Ratings section"));

  std::unordered_map<uint32_t, base::GeoObjectId> featureToOsmId;
  if (!ParseFeatureIdToOsmIdMapping(osmToFeatureFilename, featureToOsmId))
    return false;

  UGCTranslator translator(srcDbFilename);
  std::vector<uint8_t> content;
  bool haveUgc = false;
  uint8_t constexpr kNoRating = 0;

  feature::ForEachFeature(mwmFile, [&](FeatureType & f, uint32_t featureId) {
    ugc::UGC ugc;
    auto const it = featureToOsmId.find(featureId);
    // Non-OSM features (coastlines, sponsored objects) are not used.
    if (it != featureToOsmId.cend() &&
        GetUgcForFeature(it->second, feature::TypesHolder(f), translator, ugc))
    {
      content.emplace_back(ugc.GetPackedRating());
      haveUgc = true;
    }
    else
    {
      content.emplace_back(kNoRating);
    }
  });

  if (!haveUgc)
    return true;

  FilesContainerW cont(mwmFile, FileWriter::OP_WRITE_EXISTING);
  search::RankTableBuilder::Create(content, cont, RATINGS_FILE_TAG);
  return true;
}
}  // namespace generator
