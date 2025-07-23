#include "generator/popular_places_section_builder.hpp"
#include "generator/descriptions_section_builder.hpp"
#include "generator/utils.hpp"

#include "descriptions/serdes.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/rank_table.hpp"

#include "coding/csv_reader.hpp"

#include <mutex>
#include <utility>
#include <vector>

namespace generator
{
void LoadPopularPlaces(std::string const & srcFilename, PopularPlaces & places)
{
  for (auto const & row : coding::CSVRunner(coding::CSVReader(srcFilename)))
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
      LOG(LERROR, ("The popularity index value is higher than max supported value:", srcFilename, "parsed row:", row));
      return;
    }

    base::GeoObjectId id(osmId);
    auto const result = places.emplace(std::move(id), static_cast<PopularityIndex>(popularityIndex));

    if (!result.second)
    {
      LOG(LERROR, ("Popular place duplication in file:", srcFilename, "parsed row:", row));
      return;
    }
  }
}

namespace
{
template <class RankGetterT>
void BuildPopularPlacesImpl(std::string const & mwmFile, RankGetterT && getter)
{
  bool popularPlaceFound = false;

  auto const & placeChecker = ftypes::IsPlaceChecker::Instance();
  auto const & poiChecker = ftypes::IsPoiChecker::Instance();
  auto const & streetChecker = ftypes::IsWayChecker::Instance();

  std::vector<PopularityIndex> content;
  feature::ForEachFeature(mwmFile, [&](FeatureType & ft, uint32_t featureId)
  {
    ASSERT_EQUAL(content.size(), featureId, ());
    PopularityIndex rank = 0;

    if (placeChecker(ft) || poiChecker(ft) || streetChecker(ft))
    {
      rank = getter(ft, featureId);
      if (rank > 0)
        popularPlaceFound = true;
    }

    content.emplace_back(rank);
  });

  if (popularPlaceFound)
  {
    FilesContainerW cont(mwmFile, FileWriter::OP_WRITE_EXISTING);
    search::RankTableBuilder::Create(content, cont, POPULARITY_RANKS_FILE_TAG);
  }
}

PopularityIndex CalcRank(std::string const & str)
{
  auto const charsNum = strings::MakeUniString(str).size();
  return std::min(1.0, charsNum / 100000.0) * std::numeric_limits<PopularityIndex>::max();
}

}  // namespace

void BuildPopularPlacesFromDescriptions(std::string const & mwmFile)
{
  LOG(LINFO, ("Build Popular Places section"));

  /// @todo This routine looks ugly, should make better API.
  SingleMwmDataSource dataSource(mwmFile);
  auto handle = dataSource.GetHandle();
  auto const * value = handle.GetValue();
  if (!value->m_cont.IsExist(DESCRIPTIONS_FILE_TAG))
  {
    LOG(LWARNING, ("No descriptions section for", mwmFile));
    return;
  }

  auto readerPtr = value->m_cont.GetReader(DESCRIPTIONS_FILE_TAG);
  descriptions::Deserializer deserializer;

  std::vector<int8_t> langPriority;
  langPriority.push_back(StringUtf8Multilang::kEnglishCode);

  BuildPopularPlacesImpl(mwmFile, [&](FeatureType &, uint32_t featureId)
  {
    auto const str = deserializer.Deserialize(*readerPtr.GetPtr(), featureId, langPriority);
    return str.empty() ? 0 : CalcRank(str);
  });
}

void BuildPopularPlacesFromWikiDump(std::string const & mwmFile, std::string const & wikipediaDir,
                                    std::string const & idToWikidataPath)
{
  LOG(LINFO, ("Build Popular Places section"));

  DescriptionsCollector collector(wikipediaDir, mwmFile, idToWikidataPath);
  BuildPopularPlacesImpl(mwmFile, [&](FeatureType & ft, uint32_t featureId) -> PopularityIndex
  {
    collector(ft.GetMetadata().GetWikiURL(), featureId);

    if (collector.m_collection.m_features.empty())
      return 0;

    // If was added
    auto & data = collector.m_collection.m_features.back();
    if (data.m_ftIndex == featureId)
    {
      for (auto const & [lang, idx] : data.m_strIndices)
        if (lang == StringUtf8Multilang::kEnglishCode)
          return CalcRank(collector.m_collection.m_strings[idx]);
    }

    return 0;
  });
}

PopularPlaces const & GetOrLoadPopularPlaces(std::string const & filename)
{
  static std::mutex m;
  static std::unordered_map<std::string, PopularPlaces> placesStorage;

  std::lock_guard<std::mutex> lock(m);
  auto const it = placesStorage.find(filename);
  if (it != placesStorage.cend())
    return it->second;

  PopularPlaces places;
  LoadPopularPlaces(filename, places);
  auto const eIt = placesStorage.emplace(filename, places);
  return eIt.first->second;
}
}  // namespace generator
