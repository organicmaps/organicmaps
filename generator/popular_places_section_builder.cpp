#include "generator/popular_places_section_builder.hpp"

#include "generator/gen_mwm_info.hpp"
#include "generator/ugc_translator.hpp"
#include "generator/utils.hpp"

#include "ugc/binary/index_ugc.hpp"
#include "ugc/binary/serdes.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/ftraits.hpp"
#include "indexer/rank_table.hpp"

#include "base/geo_object_id.hpp"
#include "base/string_utils.hpp"

#include <cstdint>
#include <limits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
using PopularityIndex = uint8_t;
using PopularPlaces = std::unordered_map<base::GeoObjectId, PopularityIndex>;

void LoadPopularPlaces(std::string const & srcFilename, PopularPlaces & places)
{
  coding::CSVReader reader;
  auto const fileReader = FileReader(srcFilename);
  reader.Read(fileReader, [&places, &srcFilename](coding::CSVReader::Row const & row)
  {
    size_t constexpr kOsmIdPos = 0;
    size_t constexpr kPopularityIndexPos = 1;

    ASSERT_EQUAL(row.size(), 2, ());

    uint64_t osmId;
    if (!strings::to_uint64(row[kOsmIdPos], osmId))
    {
      LOG(LERROR, ("Incorrect OSM id in file:", srcFilename, "parsed row:", row));
      return;
    }

    uint32_t popularityIndex;
    if (!strings::to_uint(row[kPopularityIndexPos], popularityIndex))
    {
      LOG(LERROR, ("Incorrect popularity index in file:", srcFilename, "parsed row:", row));
      return;
    }

    if (popularityIndex > std::numeric_limits<PopularityIndex>::max())
    {
      LOG(LERROR, ("The popularity index value is higher than max supported value:", srcFilename,
                   "parsed row:", row));
      return;
    }

    base::GeoObjectId id(osmId);
    auto const result = places.emplace(std::move(id), static_cast<PopularityIndex>(popularityIndex));

    if (!result.second)
    {
      LOG(LERROR, ("Popular place duplication in file:", srcFilename, "parsed row:", row));
      return;
    }
  });
}
}  // namespace

namespace generator
{
bool BuildPopularPlacesMwmSection(std::string const & srcFilename, std::string const & mwmFile,
                                  std::string const & osmToFeatureFilename)
{
  using ugc::binary::IndexUGC;

  LOG(LINFO, ("Build Popular Places section"));

  std::unordered_map<uint32_t, base::GeoObjectId> featureIdToOsmId;
  ForEachOsmId2FeatureId(osmToFeatureFilename,
                         [&featureIdToOsmId](base::GeoObjectId const & osmId, uint32_t fId) {
                           featureIdToOsmId.emplace(fId, osmId);
                         });

  PopularPlaces places;
  LoadPopularPlaces(srcFilename, places);

  bool popularPlaceFound = false;

  std::vector<PopularityIndex> content;
  feature::ForEachFromDat(mwmFile, [&](FeatureType const & f, uint32_t featureId)
  {
    ASSERT_EQUAL(content.size(), featureId, ());

    PopularityIndex rank = 0;
    auto const it = featureIdToOsmId.find(featureId);
    // Non-OSM features (coastlines, sponsored objects) are not used.
    if (it != featureIdToOsmId.cend())
    {
      auto const placesIt = places.find(it->second);

      if (placesIt != places.cend())
      {
        popularPlaceFound = true;
        rank = placesIt->second;
      }
    }

    content.emplace_back(rank);
  });
  
  if (!popularPlaceFound)
    return true;

  FilesContainerW cont(mwmFile, FileWriter::OP_WRITE_EXISTING);
  search::RankTableBuilder::Create(content, cont, POPULARITY_RANKS_FILE_TAG);
  return true;
}
}  // namespace generator
