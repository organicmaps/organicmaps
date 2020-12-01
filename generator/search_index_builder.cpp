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
#include "indexer/feature_impl.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/postcodes_matcher.hpp"
#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"
#include "indexer/trie_builder.hpp"

#include "platform/platform.hpp"

#include "coding/map_uint32_to_val.hpp"
#include "coding/reader_writer_ops.hpp"
#include "coding/succinct_mapper.hpp"
#include "coding/writer.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "defines.hpp"

#include <algorithm>
#include <fstream>
#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std;

#define SYNONYMS_FILE "synonyms.txt"

namespace
{
class SynonymsHolder
{
public:
  explicit SynonymsHolder(string const & fPath)
  {
    ifstream stream(fPath.c_str());

    string line;
    vector<string> tokens;

    while (stream.good())
    {
      getline(stream, line);
      if (line.empty())
        continue;

      tokens.clear();
      strings::Tokenize(line, ":,", base::MakeBackInsertFunctor(tokens));

      if (tokens.size() > 1)
      {
        strings::Trim(tokens[0]);
        for (size_t i = 1; i < tokens.size(); ++i)
        {
          strings::Trim(tokens[i]);
          // For consistency, synonyms should not have any spaces.
          // For example, the hypothetical "Russia" -> "Russian Federation" mapping
          // would have the feature with name "Russia" match the request "federation". It would be wrong.
          CHECK(tokens[i].find_first_of(" \t") == string::npos, ());
          m_map.insert(make_pair(tokens[0], tokens[i]));
        }
      }
    }
  }

  template <class ToDo>
  void ForEach(string const & key, ToDo toDo) const
  {
    auto range = m_map.equal_range(key);
    while (range.first != range.second)
    {
      toDo(range.first->second);
      ++range.first;
    }
  }

private:
  unordered_multimap<string, string> m_map;
};

void GetCategoryTypes(CategoriesHolder const & categories, pair<int, int> const & scaleRange,
                      feature::TypesHolder const & types, vector<uint32_t> & result)
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
    auto indexedRange = scaleRange;
    if (scaleRange.second == scales::GetUpperScale())
      indexedRange.second = scales::GetUpperStyleScale();

    // Index only those types that are visible.
    if (feature::IsVisibleInRange(t, indexedRange))
      result.push_back(t);
  }
}

template <typename Key, typename Value>
struct FeatureNameInserter
{
  FeatureNameInserter(uint32_t index, SynonymsHolder * synonyms,
                      vector<pair<Key, Value>> & keyValuePairs, bool hasStreetType)
    : m_val(index)
    , m_synonyms(synonyms)
    , m_keyValuePairs(keyValuePairs)
    , m_hasStreetType(hasStreetType)
  {
  }

  void AddToken(uint8_t lang, strings::UniString const & s) const
  {
    strings::UniString key;
    key.reserve(s.size() + 1);
    key.push_back(lang);
    key.append(s.begin(), s.end());

    m_keyValuePairs.emplace_back(key, m_val);
  }

  // Adds search tokens for different ways of writing strasse:
  // Hauptstrasse -> Haupt strasse, Hauptstr.
  // Haupt strasse  -> Hauptstrasse, Hauptstr.
  void AddStrasseNames(signed char lang, search::QueryTokens const & tokens) const
  {
    auto static const kStrasse = strings::MakeUniString("strasse");
    auto static const kStr = strings::MakeUniString("str");
    for (size_t i = 0; i < tokens.size(); ++i)
    {
      auto const & token = tokens[i];

      if (!strings::EndsWith(token, kStrasse))
        continue;

      if (token == kStrasse)
      {
        if (i != 0)
        {
          AddToken(lang, tokens[i - 1] + kStrasse);
          AddToken(lang, tokens[i - 1] + kStr);
        }
      }
      else
      {
        auto const name = strings::UniString(token.begin(), token.end() - kStrasse.size());
        AddToken(lang, name);
        AddToken(lang, name + kStr);
      }
    }
  }

  void operator()(signed char lang, string const & name) const
  {
    strings::UniString const uniName = search::NormalizeAndSimplifyString(name);

    // split input string on tokens
    search::QueryTokens tokens;
    SplitUniString(uniName, base::MakeBackInsertFunctor(tokens), search::Delimiters());

    // add synonyms for input native string
    if (m_synonyms)
    {
      m_synonyms->ForEach(name, [&](string const & utf8str)
                          {
                            tokens.push_back(search::NormalizeAndSimplifyString(utf8str));
                          });
    }

    static_assert(search::kMaxNumTokens > 0, "");
    size_t const maxTokensCount = search::kMaxNumTokens - 1;
    if (tokens.size() > maxTokensCount)
    {
      LOG(LWARNING, ("Name has too many tokens:", name));
      tokens.resize(maxTokensCount);
    }

    if (m_hasStreetType)
    {
      search::StreetTokensFilter filter(
          [&](strings::UniString const & token, size_t /* tag */) { AddToken(lang, token); },
          false /* withMisprints */);
      for (auto const & token : tokens)
        filter.Put(token, false /* isPrefix */, 0 /* tag */);

      AddStrasseNames(lang, tokens);
    }
    else
    {
      for (auto const & token : tokens)
        AddToken(lang, token);
    }
  }

  Value m_val;
  SynonymsHolder * m_synonyms;
  vector<pair<Key, Value>> & m_keyValuePairs;
  bool m_hasStreetType = false;
};

// Returns true iff feature name was indexed as postcode and should be ignored for name indexing.
bool InsertPostcodes(FeatureType & f, function<void(strings::UniString const &)> const & fn)
{
  using namespace search;

  auto const & postBoxChecker = ftypes::IsPostBoxChecker::Instance();
  string const postcode = f.GetMetadata(feature::Metadata::FMD_POSTCODE);
  vector<string> postcodes;
  if (!postcode.empty())
    postcodes.push_back(postcode);

  bool useNameAsPostcode = false;
  if (postBoxChecker(f))
  {
    auto const & names = f.GetNames();
    if (names.CountLangs() == 1)
    {
      string defaultName;
      names.GetString(StringUtf8Multilang::kDefaultCode, defaultName);
      if (!defaultName.empty() && LooksLikePostcode(defaultName, false /* isPrefix */))
      {
        // In UK it's common practice to set outer postcode as postcode and outer + inner as ref.
        // We convert ref to name at FeatureBuilder.
        postcodes.push_back(defaultName);
        useNameAsPostcode = true;
      }
    }
  }

  for (auto const & pc : postcodes)
    SplitUniString(NormalizeAndSimplifyString(pc), fn, Delimiters());
  return useNameAsPostcode;
}

template <typename Key, typename Value>
class FeatureInserter
{
public:
  FeatureInserter(SynonymsHolder * synonyms, vector<pair<Key, Value>> & keyValuePairs,
                  CategoriesHolder const & catHolder, pair<int, int> const & scales)
    : m_synonyms(synonyms)
    , m_keyValuePairs(keyValuePairs)
    , m_categories(catHolder)
    , m_scales(scales)
  {
  }

  void operator()(FeatureType & f, uint32_t index) const
  {
    using namespace search;

    static TypesSkipper skipIndex;
    feature::TypesHolder types(f);

    if (skipIndex.SkipAlways(types))
      return;

    auto const isCountryOrState = [](auto types) {
      auto const & isLocalityChecker = ftypes::IsLocalityChecker::Instance();
      auto const localityType = isLocalityChecker.GetType(types);
      return localityType == ftypes::LocalityType::Country ||
             localityType == ftypes::LocalityType::State;
    };

    auto const & streetChecker = ftypes::IsStreetOrSquareChecker::Instance();
    bool const hasStreetType = streetChecker(types);

    // Init inserter with serialized value.
    // Insert synonyms only for countries and states (maybe will add cities in future).
    FeatureNameInserter<Key, Value> inserter(index, isCountryOrState(types) ? m_synonyms : nullptr,
                                             m_keyValuePairs, hasStreetType);

    bool const useNameAsPostcode = InsertPostcodes(
        f, [&inserter](auto const & token) { inserter.AddToken(kPostcodesLang, token); });

    if (!useNameAsPostcode)
      f.ForEachName(inserter);
    if (!f.HasName())
      skipIndex.SkipEmptyNameTypes(types);
    if (types.Empty())
      return;

    // Road number.
    if (hasStreetType)
    {
      for (auto const & shield : feature::GetRoadShieldsNames(f.GetRoadNumber()))
        inserter(StringUtf8Multilang::kDefaultCode, shield);
    }

    if (ftypes::IsAirportChecker::Instance()(types))
    {
      string const iata = f.GetMetadata(feature::Metadata::FMD_AIRPORT_IATA);
      if (!iata.empty())
        inserter(StringUtf8Multilang::kDefaultCode, iata);
    }

    // Index operator to support "Sberbank ATM" for objects with amenity=atm and operator=Sberbank.
    string const op = f.GetMetadata(feature::Metadata::FMD_OPERATOR);
    if (!op.empty())
      inserter(StringUtf8Multilang::kDefaultCode, op);

    string const brand = f.GetMetadata(feature::Metadata::FMD_BRAND);
    if (!brand.empty())
    {
      auto const & brands = indexer::GetDefaultBrands();
      brands.ForEachNameByKey(brand, [&inserter](indexer::BrandsHolder::Brand::Name const & name) {
        inserter(name.m_locale, name.m_name);
      });
    }

    Classificator const & c = classif();

    vector<uint32_t> categoryTypes;
    GetCategoryTypes(m_categories, m_scales, types, categoryTypes);

    // add names of categories of the feature
    for (uint32_t t : categoryTypes)
      inserter.AddToken(kCategoriesLang, FeatureTypeToString(c.GetIndexForType(t)));
  }

private:
  SynonymsHolder * m_synonyms;
  vector<pair<Key, Value>> & m_keyValuePairs;

  CategoriesHolder const & m_categories;

  pair<int, int> m_scales;
};

template <typename Key, typename Value>
void AddFeatureNameIndexPairs(FeaturesVectorTest const & features,
                              CategoriesHolder const & categoriesHolder,
                              vector<pair<Key, Value>> & keyValuePairs)
{
  feature::DataHeader const & header = features.GetHeader();

  unique_ptr<SynonymsHolder> synonyms;
  if (header.GetType() == feature::DataHeader::MapType::World)
    synonyms.reset(new SynonymsHolder(base::JoinPath(GetPlatform().ResourcesDir(), SYNONYMS_FILE)));

  features.GetVector().ForEach(FeatureInserter<Key, Value>(
      synonyms.get(), keyValuePairs, categoriesHolder, header.GetScaleRange()));
}

void ReadAddressData(string const & filename, vector<feature::AddressData> & addrs)
{
  FileReader reader(filename);
  ReaderSource<FileReader> src(reader);
  while (src.Size() > 0)
  {
    addrs.push_back({});
    addrs.back().DeserializeFromMwmTmp(src);
  }
}

bool GetStreetIndex(search::MwmContext & ctx, uint32_t featureID, string const & streetName,
                    uint32_t & result)
{
  bool const hasStreet = !streetName.empty();
  if (hasStreet)
  {
    auto ft = ctx.GetFeature(featureID);
    CHECK(ft, ());

    using TStreet = search::ReverseGeocoder::Street;
    vector<TStreet> streets;
    search::ReverseGeocoder::GetNearbyStreets(ctx, feature::GetCenter(*ft),
                                              true /* includeSquaresAndSuburbs */, streets);

    auto const res = search::ReverseGeocoder::GetMatchedStreetIndex(streetName, streets);

    if (res)
    {
      result = *res;
      return true;
    }
  }

  result = hasStreet ? 1 : 0;
  return false;
}

void BuildAddressTable(FilesContainerR & container, string const & addressDataFile, Writer & writer,
                       uint32_t threadsCount)
{
  vector<feature::AddressData> addrs;
  ReadAddressData(addressDataFile, addrs);

  uint32_t const featuresCount = base::checked_cast<uint32_t>(addrs.size());

  // Initialize temporary source for the current mwm file.
  FrozenDataSource dataSource;
  MwmSet::MwmId mwmId;
  {
    auto const regResult =
        dataSource.RegisterMap(platform::LocalCountryFile::MakeTemporary(container.GetFileName()));
    ASSERT_EQUAL(regResult.second, MwmSet::RegResult::Success, ());
    mwmId = regResult.first;
  }

  vector<unique_ptr<search::MwmContext>> contexts(threadsCount);

  uint32_t address = 0, missing = 0;

  uint32_t const kEmptyResult = uint32_t(-1);
  vector<uint32_t> results(featuresCount, kEmptyResult);

  mutex resMutex;

  // Thread working function.
  auto const fn = [&](uint32_t threadIdx) {
    uint64_t const fc = static_cast<uint64_t>(featuresCount);
    uint32_t const beg = static_cast<uint32_t>(fc * threadIdx / threadsCount);
    uint32_t const end = static_cast<uint32_t>(fc * (threadIdx + 1) / threadsCount);

    for (uint32_t i = beg; i < end; ++i)
    {
      uint32_t streetIndex;
      bool const found = GetStreetIndex(
          *(contexts[threadIdx]), i, addrs[i].Get(feature::AddressData::Type::Street), streetIndex);

      lock_guard<mutex> guard(resMutex);

      if (found)
      {
        results[i] = streetIndex;
        ++address;
      }
      else if (streetIndex > 0)
      {
        ++missing;
        ++address;
      }
    }
  };

  // Prepare threads and mwm contexts for each thread.
  vector<thread> threads;
  for (size_t i = 0; i < threadsCount; ++i)
  {
    auto handle = dataSource.GetMwmHandleById(mwmId);
    contexts[i] = make_unique<search::MwmContext>(move(handle));
    threads.emplace_back(fn, i);
  }

  // Wait for thread's finish.
  for (auto & t : threads)
    t.join();

  // Flush results to disk.
  {
    search::HouseToStreetTableBuilder builder;
    uint32_t houseToStreetCount = 0;
    for (size_t i = 0; i < results.size(); ++i)
    {
      if (results[i] != kEmptyResult)
      {
        builder.Put(base::asserted_cast<uint32_t>(i), results[i]);
        ++houseToStreetCount;
      }
    }

    builder.Freeze(writer);

    LOG(LINFO, ("Address: BuildingToStreet entries count:", houseToStreetCount));
  }

  double matchedPercent = 100;
  if (address > 0)
    matchedPercent = 100.0 * (1.0 - static_cast<double>(missing) / static_cast<double>(address));
  LOG(LINFO, ("Address: Matched percent", matchedPercent, "Total:", address, "Missing:", missing));
}
}  // namespace

namespace indexer
{
void BuildSearchIndex(FilesContainerR & container, Writer & indexWriter);

bool BuildSearchIndexFromDataFile(string const & country, feature::GenerateInfo const & info,
                                  bool forceRebuild, uint32_t threadsCount)
{
  Platform & platform = GetPlatform();

  auto const filename = info.GetTargetFileName(country, DATA_FILE_EXTENSION);
  FilesContainerR readContainer(platform.GetReader(filename, "f"));
  if (readContainer.IsExist(SEARCH_INDEX_FILE_TAG) && !forceRebuild)
    return true;

  string const indexFilePath = filename + "." + SEARCH_INDEX_FILE_TAG EXTENSION_TMP;
  string const addrFilePath = filename + "." + SEARCH_ADDRESS_FILE_TAG EXTENSION_TMP;
  SCOPE_GUARD(indexFileGuard, bind(&FileWriter::DeleteFileX, indexFilePath));
  SCOPE_GUARD(addrFileGuard, bind(&FileWriter::DeleteFileX, addrFilePath));

  try
  {
    {
      FileWriter writer(indexFilePath);
      BuildSearchIndex(readContainer, writer);
      LOG(LINFO, ("Search index size =", writer.Size()));
    }
    if (filename != WORLD_FILE_NAME && filename != WORLD_COASTS_FILE_NAME)
    {
      FileWriter writer(addrFilePath);
      auto const addrsFile = info.GetIntermediateFileName(country + DATA_FILE_EXTENSION, TEMP_ADDR_FILENAME);
      BuildAddressTable(readContainer, addrsFile, writer, threadsCount);
      LOG(LINFO, ("Search address table size =", writer.Size()));
    }
    {
      // Separate scopes because FilesContainerW cannot write two sections at once.
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
        header.m_indexSize =
            base::asserted_cast<uint32_t>(writer->Pos() - header.m_indexOffset - startOffset);

        auto const endOffset = writer->Pos();
        writer->Seek(startOffset);
        header.Serialize(*writer);
        writer->Seek(endOffset);
      }

      {
        FilesContainerW writeContainer(readContainer.GetFileName(), FileWriter::OP_WRITE_EXISTING);
        writeContainer.Write(addrFilePath, SEARCH_ADDRESS_FILE_TAG);
      }
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

  vector<pair<Key, Value>> searchIndexKeyValuePairs;
  AddFeatureNameIndexPairs(features, categoriesHolder, searchIndexKeyValuePairs);

  sort(searchIndexKeyValuePairs.begin(), searchIndexKeyValuePairs.end());
  LOG(LINFO, ("End sorting strings:", timer.ElapsedSeconds()));

  trie::Build<Writer, Key, ValueList<Value>, SingleValueSerializer<Value>>(
      indexWriter, serializer, searchIndexKeyValuePairs);

  LOG(LINFO, ("End building search index, elapsed seconds:", timer.ElapsedSeconds()));
}
}  // namespace indexer
