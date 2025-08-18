#include "generator/search_index_builder.hpp"

#include "search/common.hpp"
#include "search/house_to_street_table.hpp"
#include "search/mwm_context.hpp"
#include "search/reverse_geocoder.hpp"
#include "search/search_index_header.hpp"
#include "search/search_index_values.hpp"
#include "search/search_trie.hpp"
#include "search/types_skipper.hpp"

#include "indexer/brands_holder.hpp"
#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"
#include "indexer/data_source.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/postcodes_matcher.hpp"
#include "indexer/road_shields_parser.hpp"
#include "indexer/scales_patch.hpp"
#include "indexer/search_string_utils.hpp"
#include "indexer/trie_builder.hpp"

#include "platform/platform.hpp"

#include "coding/reader_writer_ops.hpp"
#include "coding/succinct_mapper.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/stats.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "defines.hpp"

#include <algorithm>
#include <fstream>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

namespace indexer
{
using namespace strings;
using namespace search;

SynonymsHolder::SynonymsHolder() : SynonymsHolder(base::JoinPath(GetPlatform().ResourcesDir(), "synonyms.txt")) {}

SynonymsHolder::SynonymsHolder(std::string const & fPath)
{
  std::ifstream stream(fPath.c_str());

  std::string line;
  while (stream.good())
  {
    std::getline(stream, line);
    if (line.empty())
      continue;

    auto tokens = strings::Tokenize<std::string>(line, ":,");
    size_t const count = tokens.size();
    if (count > 1)
    {
      strings::Trim(tokens[0]);
      auto & vec = m_map[tokens[0]];
      vec.reserve(count - 1);

      for (size_t i = 1; i < count; ++i)
      {
        strings::Trim(tokens[i]);
        // For consistency, synonyms should not have any spaces.
        // For example, the hypothetical "Russia" -> "Russian Federation" mapping
        // would have the feature with name "Russia" match the request "federation". It would be wrong.
        CHECK(tokens[i].find_first_of(" \t") == std::string::npos, ());
        vec.push_back(std::move(tokens[i]));
      }
    }
  }
}

namespace
{
template <class FnT>
void GetCategoryTypes(CategoriesHolder const & categories, std::pair<int, int> scaleRange,
                      feature::TypesHolder const & types, FnT const & fn)
{
  for (uint32_t t : types)
  {
    // Truncate |t| up to 2 levels and choose the best category match to find explicit category if
    // any and not distinguish types like highway-primary-bridge and highway-primary-tunnel or
    // amenity-parking-fee and amenity-parking-underground-fee if we do not have such explicit
    // categories.

    for (uint8_t level = ftype::GetLevel(t); level >= 2; --level)
    {
      ftype::TruncValue(t, level);
      if (categories.IsTypeExist(t))
        break;
    }

    // Only categorized types will be added to index.
    if (!categories.IsTypeExist(t))
      continue;

    // Drawable scale must be normalized to indexer scales.
    scaleRange.second = scales::PatchMaxDrawableScale(scaleRange.second);

    // Index only those types that are visible.
    if (feature::IsVisibleInRange(t, scaleRange))
      fn(t);
  }
}

template <class ContT>
class FeatureNameInserter
{
  String2StringMap const & m_suffixes;

  base::TopStatsCounter<std::string> m_stats;

public:
  explicit FeatureNameInserter(ContT & keyValuePairs) : m_suffixes(GetDACHStreets()), m_keyValuePairs(keyValuePairs) {}
  ~FeatureNameInserter()
  {
    LOG(LINFO, ("Top street's name tokens:"));
    m_stats.PrintTop(10);
  }

  void SetFeature(uint32_t index, SynonymsHolder const * synonyms, bool hasStreetType)
  {
    m_val = index;
    m_synonyms = synonyms;
    m_hasStreetType = hasStreetType;
  }

  void AddToken(uint8_t lang, UniString const & s) const
  {
    UniString key;
    key.reserve(s.size() + 1);
    key.push_back(lang);
    key.append(s.begin(), s.end());

    m_keyValuePairs.emplace_back(key, m_val);
  }

  // Adds search tokens for different ways of writing strasse:
  // Hauptstrasse -> Haupt strasse, Hauptstr.
  // Haupt strasse  -> Hauptstrasse, Hauptstr.
  void AddDACHNames(int8_t lang, std::vector<UniString> const & tokens) const
  {
    for (size_t i = 0; i < tokens.size(); ++i)
    {
      auto const & token = tokens[i];

      for (auto const & sx : m_suffixes)
      {
        if (!EndsWith(token, sx.first))
          continue;

        // We expect that suffixes are street synonyms, so no need to add them separately into index.
        ASSERT(IsStreetSynonym(sx.first) || IsStreetSynonym(sx.second), (sx.first));

        if (token == sx.first)
        {
          if (i != 0)
          {
            AddToken(lang, tokens[i - 1] + sx.first);
            AddToken(lang, tokens[i - 1] + sx.second);
          }
        }
        else
        {
          auto const name = UniString(token.begin(), token.end() - sx.first.size());
          AddToken(lang, name);
          AddToken(lang, name + sx.second);
        }
      }
    }
  }

  void operator()(int8_t lang, std::string_view name)
  {
    /// @todo No problem here if we will have duplicating tokens? (POI name like "Step by Step").
    auto tokens = NormalizeAndTokenizeString(name);

    // add synonyms for input native string
    if (m_synonyms)
    {
      /// @todo Avoid creating temporary std::string.
      m_synonyms->ForEach(std::string(name),
                          [&](std::string const & utf8str) { tokens.push_back(NormalizeAndSimplifyString(utf8str)); });
    }

    static_assert(kMaxNumTokens > 0);
    size_t const maxTokensCount = kMaxNumTokens - 1;
    if (tokens.size() > maxTokensCount)
    {
      LOG(LWARNING, ("Name has too many tokens:", name));
      tokens.resize(maxTokensCount);
    }

    if (m_hasStreetType)
    {
      StreetTokensFilter filter([this, lang](UniString const & token, size_t) { AddToken(lang, token); },
                                false /* withMisprints */);

      for (auto const & token : tokens)
      {
        if (m_statsEnabled)
          m_stats.Add(strings::ToUtf8(token));

        filter.Put(token, false /* isPrefix */, 0 /* tag */);
      }

      AddDACHNames(lang, tokens);
    }
    else
    {
      for (auto const & token : tokens)
        AddToken(lang, token);
    }
  }

  bool m_statsEnabled = false;

private:
  uint32_t m_val;
  SynonymsHolder const * m_synonyms;
  ContT & m_keyValuePairs;
  bool m_hasStreetType = false;
};

// Returns true iff feature name was indexed as postcode and should be ignored for name indexing.
template <class FnT>
bool InsertPostcodes(FeatureType & f, FnT && fn)
{
  using namespace search;

  // PostBox only, not IsPostPoiChecker?
  auto const & postBoxChecker = ftypes::IsPostBoxChecker::Instance();
  auto const postcode = f.GetMetadata(feature::Metadata::FMD_POSTCODE);

  if (!postcode.empty())
    ForEachNormalizedToken(postcode, fn);

  bool useNameAsPostcode = false;
  if (postBoxChecker(f))
  {
    auto const & names = f.GetNames();
    if (names.CountLangs() == 1)
    {
      std::string_view defaultName;
      names.GetString(StringUtf8Multilang::kDefaultCode, defaultName);
      if (!defaultName.empty() && LooksLikePostcode(defaultName, false /* isPrefix */))
      {
        // In UK it's common practice to set outer postcode as postcode and outer + inner as ref.
        // We convert ref to name at FeatureBuilder.
        ForEachNormalizedToken(defaultName, fn);
        useNameAsPostcode = true;
      }
    }
  }

  return useNameAsPostcode;
}

template <class ContT>
class FeatureInserter
{
public:
  FeatureInserter(SynonymsHolder * synonyms, ContT & keyValuePairs, CategoriesHolder const & catHolder,
                  std::pair<int, int> const & scales)
    : m_synonyms(synonyms)
    , m_categories(catHolder)
    , m_scales(scales)
    , m_inserter(keyValuePairs)
  {}

  void operator()(FeatureType & f, uint32_t index)
  {
    feature::TypesHolder types(f);

    if (m_skipIndex.SkipAlways(types))
      return;
    if (m_skipIndex.SkipSpecialNames(types, f.GetName(StringUtf8Multilang::kDefaultCode)))
      return;

    SynonymsHolder const * synonyms = nullptr;
    if (m_synonyms)
    {
      // Insert synonyms only for countries and states (maybe will add cities in future).
      if (SynonymsHolder::CanApply(types))
        synonyms = m_synonyms;
    }

    bool const hasStreetType = ftypes::IsStreetOrSquareChecker::Instance()(types);

    // Init inserter with Feature's index.
    m_inserter.SetFeature(index, synonyms, hasStreetType);

    bool const useNameAsPostcode =
        InsertPostcodes(f, [this](auto const & token) { m_inserter.AddToken(search::kPostcodesLang, token); });

    if (!useNameAsPostcode)
    {
      m_inserter.m_statsEnabled = true;
      f.ForEachName(m_inserter);
      m_inserter.m_statsEnabled = false;
    }

    // Road number.
    if (hasStreetType)
      for (auto const & shield : ftypes::GetRoadShieldsNames(f))
        m_inserter(StringUtf8Multilang::kDefaultCode, shield);

    if (ftypes::IsAirportChecker::Instance()(types))
    {
      auto const iata = f.GetMetadata(feature::Metadata::FMD_AIRPORT_IATA);
      if (!iata.empty())
        m_inserter(StringUtf8Multilang::kDefaultCode, iata);
    }

    // Index operator to support "Sberbank ATM" for objects with amenity=atm and operator=Sberbank.
    auto const op = f.GetMetadata(feature::Metadata::FMD_OPERATOR);
    if (!op.empty())
      m_inserter(StringUtf8Multilang::kDefaultCode, op);

    auto const brand = f.GetMetadata(feature::Metadata::FMD_BRAND);
    if (!brand.empty())
    {
      ForEachLocalizedBrands(
          brand, [this](BrandsHolder::Brand::Name const & name) { m_inserter(name.m_locale, name.m_name); });
    }

    // Check for empty name just before categories indexing. After postcodes, and other meta ..
    if (!f.HasName())
      m_skipIndex.SkipEmptyNameTypes(types);
    if (types.Empty())
      return;

    Classificator const & c = classif();
    GetCategoryTypes(m_categories, m_scales, types, [this, &c](uint32_t t)
    { m_inserter.AddToken(search::kCategoriesLang, search::FeatureTypeToString(c.GetIndexForType(t))); });
  }

private:
  SynonymsHolder * m_synonyms;

  CategoriesHolder const & m_categories;
  std::pair<int, int> m_scales;

  search::TypesSkipper m_skipIndex;
  FeatureNameInserter<ContT> m_inserter;
};

template <class ContT>
void AddFeatureNameIndexPairs(FeaturesVectorTest const & features, CategoriesHolder const & categoriesHolder,
                              ContT & keyValuePairs)
{
  feature::DataHeader const & header = features.GetHeader();

  std::unique_ptr<SynonymsHolder> synonyms;
  if (header.GetType() == feature::DataHeader::MapType::World)
    synonyms = std::make_unique<SynonymsHolder>();

  features.GetVector().ForEach(
      FeatureInserter(synonyms.get(), keyValuePairs, categoriesHolder, header.GetScaleRange()));
}

void ReadAddressData(std::string const & filename, std::vector<feature::AddressData> & addrs)
{
  FileReader reader(filename);
  ReaderSource<FileReader> src(reader);
  while (src.Size() > 0)
  {
    addrs.push_back({});
    addrs.back().DeserializeFromMwmTmp(src);
  }
}

template <class ObjT, class GetKeyFnT>
std::optional<uint32_t> MatchObjectByName(std::string_view keyName, std::vector<ObjT> const & objs, GetKeyFnT && getKey)
{
  size_t constexpr kSimilarityThresholdPercent = 10;

  // Find the exact match or the best match in kSimilarityTresholdPercent limit.
  uint32_t result;
  size_t minPercent = kSimilarityThresholdPercent + 1;

  auto const key = getKey(keyName);
  for (auto const & obj : objs)
  {
    bool fullMatchFound = false;
    obj.m_multilangName.ForEach([&](int8_t lang, std::string_view name)
    {
      if (fullMatchFound)
        return;

      // Skip _non-language_ names for street<->address matching.
      if (StringUtf8Multilang::IsAltOrOldName(lang))
        return;

      strings::UniString const actual = getKey(name);
      size_t const editDistance = strings::EditDistance(key.begin(), key.end(), actual.begin(), actual.end());

      if (editDistance == 0)
      {
        result = obj.m_id.m_index;
        fullMatchFound = true;
        return;
      }

      if (actual.empty())
        return;

      size_t const percent = editDistance * 100 / actual.size();
      if (percent < minPercent)
      {
        result = obj.m_id.m_index;
        minPercent = percent;
      }
    });

    if (fullMatchFound)
      return result;
  }

  if (minPercent <= kSimilarityThresholdPercent)
    return result;
  return {};
}

void BuildAddressTable(FilesContainerR & container, std::string const & addressDataFile, Writer & streetsWriter,
                       Writer & placesWriter, uint32_t threadsCount)
{
  std::vector<feature::AddressData> addrs;
  ReadAddressData(addressDataFile, addrs);

  auto const featuresCount = base::checked_cast<uint32_t>(addrs.size());

  // Initialize temporary source for the current mwm file.
  FrozenDataSource dataSource;
  MwmSet::MwmId mwmId;
  {
    auto const regResult = dataSource.RegisterMap(platform::LocalCountryFile::MakeTemporary(container.GetFileName()));
    ASSERT_EQUAL(regResult.second, MwmSet::RegResult::Success, ());
    mwmId = regResult.first;
  }

  std::vector<std::unique_ptr<search::MwmContext>> contexts(threadsCount);

  std::atomic<uint32_t> address = 0;
  std::atomic<uint32_t> missing = 0;

  /// @see Addr_Street_Place test for checking constants.
  double constexpr kStreetRadiusM = 2000;
  double constexpr kPlaceRadiusM = 4000;

  std::vector<uint32_t> streets(featuresCount, kInvalidFeatureId);
  std::vector<uint32_t> places(featuresCount, kInvalidFeatureId);

  // Thread working function.
  auto const fn = [&](uint32_t threadIdx)
  {
    auto const fc = static_cast<uint64_t>(featuresCount);
    auto const beg = static_cast<uint32_t>(fc * threadIdx / threadsCount);
    auto const end = static_cast<uint32_t>(fc * (threadIdx + 1) / threadsCount);

    for (uint32_t i = beg; i < end; ++i)
    {
      auto const street = addrs[i].Get(feature::AddressData::Type::Street);
      auto const place = addrs[i].Get(feature::AddressData::Type::Place);
      if (street.empty() && place.empty())
        continue;

      ++address;

      std::optional<uint32_t> streetId, placeId;

      auto ft = contexts[threadIdx]->GetFeature(i);
      CHECK(ft, ());
      auto const center = feature::GetCenter(*ft);

      if (!street.empty())
      {
        auto const streets = search::ReverseGeocoder::GetNearbyStreets(*contexts[threadIdx], center, kStreetRadiusM);
        streetId = MatchObjectByName(street, streets, [](std::string_view name)
        { return search::GetStreetNameAsKey(name, false /* ignoreStreetSynonyms */); });

        if (!streetId)
        {
          streetId = MatchObjectByName(street, streets, [](std::string_view name)
          { return search::GetStreetNameAsKey(name, true /* ignoreStreetSynonyms */); });
        }
      }

      if (!place.empty())
      {
        auto const places = search::ReverseGeocoder::GetNearbyPlaces(*contexts[threadIdx], center, kPlaceRadiusM);
        placeId = MatchObjectByName(place, places, [](std::string_view name) { return strings::MakeUniString(name); });
      }

      if (streetId || placeId)
      {
        if (streetId)
          streets[i] = *streetId;
        if (placeId)
          places[i] = *placeId;
      }
      else
        ++missing;
    }
  };

  // Prepare threads and mwm contexts for each thread.
  std::vector<std::thread> threads;
  for (size_t i = 0; i < threadsCount; ++i)
  {
    auto handle = dataSource.GetMwmHandleById(mwmId);
    contexts[i] = std::make_unique<search::MwmContext>(std::move(handle));
    threads.emplace_back(fn, i);
  }

  // Wait for thread's finish.
  for (auto & t : threads)
    t.join();

  // Flush results to disk.
  auto const flushToWriter = [](std::vector<uint32_t> const & results, Writer & writer)
  {
    search::HouseToStreetTableBuilder builder;
    uint32_t count = 0;
    for (size_t i = 0; i < results.size(); ++i)
    {
      if (results[i] != kInvalidFeatureId)
      {
        builder.Put(base::asserted_cast<uint32_t>(i), results[i]);
        ++count;
      }
    }

    builder.Freeze(writer);
    return count;
  };

  LOG(LINFO, ("Saved streets entries number:", flushToWriter(streets, streetsWriter)));
  LOG(LINFO, ("Saved places entries number:", flushToWriter(places, placesWriter)));

  double matchedPercent = 100;
  if (address > 0)
    matchedPercent = 100.0 * (1.0 - static_cast<double>(missing) / static_cast<double>(address));
  LOG(LINFO, ("Matched addresses percent:", matchedPercent, "Total:", address, "Missing:", missing));
}
}  // namespace

void BuildSearchIndex(FilesContainerR & container, Writer & indexWriter);

bool BuildSearchIndexFromDataFile(std::string const & country, feature::GenerateInfo const & info, bool forceRebuild,
                                  uint32_t threadsCount)
{
  Platform & platform = GetPlatform();

  auto const filename = info.GetTargetFileName(country, DATA_FILE_EXTENSION);
  FilesContainerR readContainer(platform.GetReader(filename, "f"));
  if (readContainer.IsExist(SEARCH_INDEX_FILE_TAG) && !forceRebuild)
    return true;

  auto const indexFilePath = filename + "." + SEARCH_INDEX_FILE_TAG EXTENSION_TMP;
  auto const streetsFilePath = filename + "." + FEATURE2STREET_FILE_TAG EXTENSION_TMP;
  auto const placesFilePath = filename + "." + FEATURE2PLACE_FILE_TAG EXTENSION_TMP;
  SCOPE_GUARD(indexFileGuard, std::bind(&FileWriter::DeleteFileX, indexFilePath));
  SCOPE_GUARD(streetsFileGuard, std::bind(&FileWriter::DeleteFileX, streetsFilePath));
  SCOPE_GUARD(placesFileGuard, std::bind(&FileWriter::DeleteFileX, placesFilePath));

  try
  {
    {
      FileWriter writer(indexFilePath);
      BuildSearchIndex(readContainer, writer);
      LOG(LINFO, ("Search index size =", writer.Size()));
    }

    if (filename != WORLD_FILE_NAME && filename != WORLD_COASTS_FILE_NAME)
    {
      FileWriter streetsWriter(streetsFilePath);
      FileWriter placesWriter(placesFilePath);
      auto const addrsFile = info.GetIntermediateFileName(country + DATA_FILE_EXTENSION, TEMP_ADDR_EXTENSION);
      BuildAddressTable(readContainer, addrsFile, streetsWriter, placesWriter, threadsCount);
      LOG(LINFO, ("Streets table size:", streetsWriter.Size(), "; Places table size:", placesWriter.Size()));
    }

    // Separate scopes because FilesContainerW can't write two sections at once.
    {
      FilesContainerW writeContainer(readContainer.GetFileName(), FileWriter::OP_WRITE_EXISTING);
      auto writer = writeContainer.GetWriter(SEARCH_INDEX_FILE_TAG);
      size_t const startOffset = writer->Pos();
      CHECK(coding::IsAlign8(startOffset), ());

      search::SearchIndexHeader header;
      header.Serialize(*writer);

      uint64_t bytesWritten = writer->Pos();
      coding::WritePadding(*writer, bytesWritten);

      header.m_indexOffset = base::asserted_cast<uint32_t>(writer->Pos() - startOffset);
      rw_ops::Reverse(FileReader(indexFilePath), *writer);
      header.m_indexSize = base::asserted_cast<uint32_t>(writer->Pos() - header.m_indexOffset - startOffset);

      auto const endOffset = writer->Pos();
      writer->Seek(startOffset);
      header.Serialize(*writer);
      writer->Seek(endOffset);
    }

    /// @todo FilesContainerW::Write with section overriding invalidates current container instance
    /// (@see FilesContainerW::GetWriter), thus we can't make 2 consecutive Write calls.
    {
      FilesContainerW writeContainer(readContainer.GetFileName(), FileWriter::OP_WRITE_EXISTING);
      writeContainer.Write(streetsFilePath, FEATURE2STREET_FILE_TAG);
    }
    {
      FilesContainerW writeContainer(readContainer.GetFileName(), FileWriter::OP_WRITE_EXISTING);
      writeContainer.Write(placesFilePath, FEATURE2PLACE_FILE_TAG);
    }
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Error while reading file:", e.Msg()));
    return false;
  }
  catch (Writer::Exception const & e)
  {
    LOG(LERROR, ("Error writing index file:", e.Msg()));
    return false;
  }

  return true;
}

void BuildSearchIndex(FilesContainerR & container, Writer & indexWriter)
{
  using Key = strings::UniString;
  using Value = Uint64IndexValue;

  LOG(LINFO, ("Start building search index for", container.GetFileName()));
  base::Timer timer;

  auto const & categoriesHolder = GetDefaultCategories();

  FeaturesVectorTest features(container);
  SingleValueSerializer<Value> serializer;

  std::vector<std::pair<Key, Value>> searchIndexKeyValuePairs;
  AddFeatureNameIndexPairs(features, categoriesHolder, searchIndexKeyValuePairs);

  std::sort(searchIndexKeyValuePairs.begin(), searchIndexKeyValuePairs.end());
  LOG(LINFO, ("End sorting strings:", timer.ElapsedSeconds()));

  trie::Build<Writer, Key, ValueList<Value>, SingleValueSerializer<Value>>(indexWriter, serializer,
                                                                           searchIndexKeyValuePairs);

  LOG(LINFO, ("End building search index, elapsed seconds:", timer.ElapsedSeconds()));
}
}  // namespace indexer
