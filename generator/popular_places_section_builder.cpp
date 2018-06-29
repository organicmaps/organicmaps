#include "generator/popular_places_section_builder.hpp"

#include "generator/gen_mwm_info.hpp"
#include "generator/ugc_translator.hpp"

#include "ugc/binary/index_ugc.hpp"
#include "ugc/binary/serdes.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/ftraits.hpp"
#include "indexer/rank_table.hpp"

#include "base/osm_id.hpp"
#include "base/string_utils.hpp"

#include <unordered_map>
#include <utility>

namespace
{
using PopularityIndex = uint8_t;
using PopularPlaces = std::unordered_map<osm::Id, PopularityIndex, osm::HashId>;

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
    osm::Id id(osmId);
    auto const result = places.emplace(id, static_cast<PopularityIndex>(popularityIndex));

    if (result.second == false)
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

  gen::OsmID2FeatureID osmIdsToFeatureIds;
  if (!osmIdsToFeatureIds.ReadFromFile(osmToFeatureFilename))
    return false;

  std::unordered_map<uint32_t, osm::Id> featureToOsmId;
  osmIdsToFeatureIds.ForEach([&featureToOsmId](gen::OsmID2FeatureID::ValueT const & p) {
    featureToOsmId.emplace(p.second /* feature id */, p.first /* osm id */);
  });

  PopularPlaces places;
  LoadPopularPlaces(srcFilename, places);

  bool popularPlaceFound = false;

  std::vector<uint8_t> content;
  feature::ForEachFromDat(mwmFile, [&](FeatureType const & f, uint32_t featureId) {
    auto const it = featureToOsmId.find(featureId);
    CHECK(it != featureToOsmId.cend(),
          ("FeatureID", featureId, "is not found in", osmToFeatureFilename));

    uint32_t rank = 0;
    auto const placesIt = places.find(it->second);

    if (placesIt != places.cend())
    {
      popularPlaceFound = true;
      rank = placesIt->second;
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
