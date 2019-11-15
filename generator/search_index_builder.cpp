#include "search_index_builder.hpp"

#include "search/common.hpp"
#include "search/mwm_context.hpp"
#include "search/postcode_points.hpp"
#include "search/reverse_geocoder.hpp"
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

#include "storage/country_info_getter.hpp"
#include "storage/storage_defines.hpp"

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

#include <algorithm>
#include <fstream>
#include <map>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <thread>

#include "defines.hpp"

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

template <typename Key, typename Value>
void GetUKPostcodes(string const & filename, storage::CountryId const & countryId,
                    storage::CountryInfoGetter & infoGetter, vector<m2::PointD> & valueMapping,
                    vector<pair<Key, Value>> & keyValuePairs)
{
  // Original dataset uses UK National Grid UTM coordinates.
  // It was converted to WGS84 by https://pypi.org/project/OSGridConverter/.
  size_t constexpr kPostcodeIndex = 0;
  size_t constexpr kLatIndex = 1;
  size_t constexpr kLongIndex = 2;
  size_t constexpr kDatasetCount = 3;

  ifstream data;
  data.exceptions(fstream::failbit | fstream::badbit);
  data.open(filename);
  data.exceptions(fstream::badbit);

  string line;
  size_t index = 0;
  while (getline(data, line))
  {
    vector<string> fields;
    strings::ParseCSVRow(line, ',', fields);
    CHECK_EQUAL(fields.size(), kDatasetCount, (line));

    double lat;
    CHECK(strings::to_double(fields[kLatIndex], lat), ());

    double lon;
    CHECK(strings::to_double(fields[kLongIndex], lon), ());

    auto const p = mercator::FromLatLon(lat, lon);

    vector<storage::CountryId> countries;
    infoGetter.GetRegionsCountryId(p, countries, 200.0 /* lookupRadiusM */);
    if (find(countries.begin(), countries.end(), countryId) == countries.end())
      continue;

    // UK postcodes formats are: aana naa, ana naa, an naa, ann naa, aan naa, aann naa.

    auto postcode = fields[kPostcodeIndex];
    // Do not index outer postcodes.
    if (postcode.size() < 5)
      continue;

    // Space is skipped in dataset for |aana naa| and |aann naa| to make it fit 7 symbols in csv.
    // Let's fix it here.
    if (postcode.find(' ') == string::npos)
      postcode.insert(static_cast<size_t>(postcode.size() - 3), " ");

    CHECK_EQUAL(valueMapping.size(), index, ());
    valueMapping.push_back(p);
    keyValuePairs.emplace_back(search::NormalizeAndSimplifyString(postcode), Value(index));
    ++index;
  }
}

// Returns true iff feature name was indexed as postcode and should be ignored for name indexing.
bool InsertPostcodes(FeatureType & f, vector<feature::AddressData> const & addrs,
                     function<void(strings::UniString const &)> const & fn)
{
  using namespace search;

  auto const & postBoxChecker = ftypes::IsPostBoxChecker::Instance();
  string const postcode = addrs[f.GetID().m_index].Get(feature::AddressData::Type::Postcode);
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
                  vector<feature::AddressData> const & addrs, CategoriesHolder const & catHolder,
                  pair<int, int> const & scales)
    : m_synonyms(synonyms)
    , m_keyValuePairs(keyValuePairs)
    , m_addrs(addrs)
    , m_categories(catHolder)
    , m_scales(scales)
  {
  }

  void operator()(FeatureType & f, uint32_t index) const
  {
    using namespace search;

    static TypesSkipper skipIndex;

    auto const isCountryOrState = [](auto types) {
      auto const & isLocalityChecker = ftypes::IsLocalityChecker::Instance();
      auto const localityType = isLocalityChecker.GetType(types);
      return localityType == ftypes::LocalityType::Country ||
             localityType == ftypes::LocalityType::State;
    };

    feature::TypesHolder types(f);

    auto const & streetChecker = ftypes::IsStreetOrSquareChecker::Instance();
    bool const hasStreetType = streetChecker(types);

    // Init inserter with serialized value.
    // Insert synonyms only for countries and states (maybe will add cities in future).
    FeatureNameInserter<Key, Value> inserter(index, isCountryOrState(types) ? m_synonyms : nullptr,
                                             m_keyValuePairs, hasStreetType);

    bool const useNameAsPostcode = InsertPostcodes(
        f, m_addrs, [&inserter](auto const & token) { inserter.AddToken(kPostcodesLang, token); });

    if (!useNameAsPostcode)
      f.ForEachName(inserter);
    if (!f.HasName())
      skipIndex.SkipEmptyNameTypes(types);
    if (types.Empty())
      return;

    // Road number.
    if (hasStreetType && !f.GetParams().ref.empty())
      inserter(StringUtf8Multilang::kDefaultCode, f.GetParams().ref);

    if (ftypes::IsAirportChecker::Instance()(types))
    {
      string const iata = f.GetMetadata().Get(feature::Metadata::FMD_AIRPORT_IATA);
      if (!iata.empty())
        inserter(StringUtf8Multilang::kDefaultCode, iata);
    }

    // Index operator to support "Sberbank ATM" for objects with amenity=atm and operator=Sberbank.
    string const op = f.GetMetadata().Get(feature::Metadata::FMD_OPERATOR);
    if (!op.empty())
      inserter(StringUtf8Multilang::kDefaultCode, op);

    string const brand = f.GetMetadata().Get(feature::Metadata::FMD_BRAND);
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

  vector<feature::AddressData> const & m_addrs;

  CategoriesHolder const & m_categories;

  pair<int, int> m_scales;
};

template <typename Key, typename Value>
void AddFeatureNameIndexPairs(FeaturesVectorTest const & features,
                              vector<feature::AddressData> const & addrs,
                              CategoriesHolder const & categoriesHolder,
                              vector<pair<Key, Value>> & keyValuePairs)
{
  feature::DataHeader const & header = features.GetHeader();

  unique_ptr<SynonymsHolder> synonyms;
  if (header.GetType() == feature::DataHeader::MapType::World)
    synonyms.reset(new SynonymsHolder(base::JoinPath(GetPlatform().ResourcesDir(), SYNONYMS_FILE)));

  features.GetVector().ForEach(FeatureInserter<Key, Value>(
      synonyms.get(), keyValuePairs, addrs, categoriesHolder, header.GetScaleRange()));
}

void ReadAddressData(FilesContainerR & container, vector<feature::AddressData> & addrs)
{
  ReaderSource<ModelReaderPtr> src(container.GetReader(TEMP_ADDR_FILE_TAG));
  while (src.Size() > 0)
  {
    addrs.push_back({});
    addrs.back().Deserialize(src);
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

void BuildAddressTable(FilesContainerR & container, vector<feature::AddressData> const & addrs,
                       Writer & writer, uint32_t threadsCount)
{
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
    // Code corresponds to the HouseToStreetTable decoding.
    MapUint32ToValueBuilder<uint32_t> builder;
    uint32_t houseToStreetCount = 0;
    for (size_t i = 0; i < results.size(); ++i)
    {
      if (results[i] != kEmptyResult)
      {
        builder.Put(base::asserted_cast<uint32_t>(i), results[i]);
        ++houseToStreetCount;
      }
    }

    // Each street id is encoded as delta from some prediction.
    // First street id in the block encoded as VarUint, all other street ids in the block
    // encoded as VarInt delta from previous id.
    auto const writeBlockCallback = [&](Writer & w, vector<uint32_t>::const_iterator begin,
                                        vector<uint32_t>::const_iterator end) {
      CHECK(begin != end, ("MapUint32ToValueBuilder should guarantee begin != end."));
      WriteVarUint(w, *begin);
      auto prevIt = begin;
      for (auto it = begin + 1; it != end; ++it)
      {
        int32_t const delta = base::asserted_cast<int32_t>(*it) - *prevIt;
        WriteVarInt(w, delta);
        prevIt = it;
      }
    };
    builder.Freeze(writer, writeBlockCallback);

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
void BuildSearchIndex(FilesContainerR & container, vector<feature::AddressData> const & addrs,
                      Writer & indexWriter);
bool BuildPostcodesImpl(FilesContainerR & container, storage::CountryId const & country,
                        string const & dataset, string const & tmpFileName,
                        storage::CountryInfoGetter & infoGetter, Writer & indexWriter);

bool BuildSearchIndexFromDataFile(string const & filename, bool forceRebuild, uint32_t threadsCount)
{
  Platform & platform = GetPlatform();

  FilesContainerR readContainer(platform.GetReader(filename, "f"));
  if (readContainer.IsExist(SEARCH_INDEX_FILE_TAG) && !forceRebuild)
    return true;

  string const indexFilePath = filename + "." + SEARCH_INDEX_FILE_TAG EXTENSION_TMP;
  string const addrFilePath = filename + "." + SEARCH_ADDRESS_FILE_TAG EXTENSION_TMP;
  SCOPE_GUARD(indexFileGuard, bind(&FileWriter::DeleteFileX, indexFilePath));
  SCOPE_GUARD(addrFileGuard, bind(&FileWriter::DeleteFileX, addrFilePath));

  try
  {
    vector<feature::AddressData> addrs;
    ReadAddressData(readContainer, addrs);
    {
      FileWriter writer(indexFilePath);
      BuildSearchIndex(readContainer, addrs, writer);
      LOG(LINFO, ("Search index size =", writer.Size()));
    }
    if (filename != WORLD_FILE_NAME && filename != WORLD_COASTS_FILE_NAME)
    {
      FileWriter writer(addrFilePath);
      BuildAddressTable(readContainer, addrs, writer, threadsCount);
      LOG(LINFO, ("Search address table size =", writer.Size()));
    }
    {
      // The behaviour of generator_tool's generate_search_index
      // is currently broken: this section is generated elsewhere
      // and is deleted here before the final step of the mwm generation
      // so it does not pollute the resulting mwm.
      // So using and deleting this section is fine when generating
      // an mwm from scratch but does not work when regenerating the
      // search index section. Comment out the call to DeleteSection
      // if you need to regenerate the search intex.
      // todo(@m) Is it possible to make it work?
      {
        FilesContainerW writeContainer(readContainer.GetFileName(), FileWriter::OP_WRITE_EXISTING);
        writeContainer.DeleteSection(TEMP_ADDR_FILE_TAG);
      }

      // Separate scopes because FilesContainerW cannot write two sections at once.
      {
        FilesContainerW writeContainer(readContainer.GetFileName(), FileWriter::OP_WRITE_EXISTING);
        auto writer = writeContainer.GetWriter(SEARCH_INDEX_FILE_TAG);
        rw_ops::Reverse(FileReader(indexFilePath), *writer);
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

bool BuildPostcodesWithInfoGetter(string const & path, string const & country,
                                  string const & datasetPath, bool forceRebuild,
                                  storage::CountryInfoGetter & infoGetter)
{
  auto const filename = base::JoinPath(path, country + DATA_FILE_EXTENSION);
  if (filename == WORLD_FILE_NAME || filename == WORLD_COASTS_FILE_NAME)
    return true;

  Platform & platform = GetPlatform();
  FilesContainerR readContainer(platform.GetReader(filename, "f"));
  if (readContainer.IsExist(POSTCODE_POINTS_FILE_TAG) && !forceRebuild)
    return true;

  string const postcodesFilePath = filename + "." + POSTCODE_POINTS_FILE_TAG EXTENSION_TMP;
  // Temporary file used to reverse trie part of postcodes section.
  string const trieTmpFilePath =
      filename + "." + POSTCODE_POINTS_FILE_TAG + "_trie" + EXTENSION_TMP;
  SCOPE_GUARD(postcodesFileGuard, bind(&FileWriter::DeleteFileX, postcodesFilePath));
  SCOPE_GUARD(trieTmpFileGuard, bind(&FileWriter::DeleteFileX, trieTmpFilePath));

  try
  {
    FileWriter writer(postcodesFilePath);
    if (!BuildPostcodesImpl(readContainer, storage::CountryId(country), datasetPath,
                            trieTmpFilePath, infoGetter, writer))
    {
      // No postcodes for country.
      return true;
    }

    LOG(LINFO, ("Postcodes section size =", writer.Size()));
    FilesContainerW writeContainer(readContainer.GetFileName(), FileWriter::OP_WRITE_EXISTING);
    writeContainer.Write(postcodesFilePath, POSTCODE_POINTS_FILE_TAG);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Error while reading file:", e.Msg()));
    return false;
  }
  catch (Writer::Exception const & e)
  {
    LOG(LERROR, ("Error writing file:", e.Msg()));
    return false;
  }

  return true;
}

bool BuildPostcodes(string const & path, string const & country, string const & datasetPath,
                    bool forceRebuild)
{
  auto const & platform = GetPlatform();
  auto infoGetter = storage::CountryInfoReader::CreateCountryInfoReader(platform);
  CHECK(infoGetter, ());
  return BuildPostcodesWithInfoGetter(path, country, datasetPath, forceRebuild, *infoGetter);
}

bool BuildPostcodesImpl(FilesContainerR & container, storage::CountryId const & country,
                        string const & dataset, string const & tmpName,
                        storage::CountryInfoGetter & infoGetter, Writer & writer)
{
  using Key = strings::UniString;
  using Value = Uint64IndexValue;

  CHECK_EQUAL(writer.Pos(), 0, ());

  search::PostcodePoints::Header header;
  header.Serialize(writer);

  uint64_t bytesWritten = writer.Pos();
  coding::WritePadding(writer, bytesWritten);

  header.m_trieOffset = base::asserted_cast<uint32_t>(writer.Pos());

  vector<pair<Key, Value>> ukPostcodesKeyValuePairs;
  vector<m2::PointD> valueMapping;
  GetUKPostcodes(dataset, country, infoGetter, valueMapping, ukPostcodesKeyValuePairs);

  if (ukPostcodesKeyValuePairs.empty())
    return false;

  sort(ukPostcodesKeyValuePairs.begin(), ukPostcodesKeyValuePairs.end());

  {
    FileWriter tmpWriter(tmpName);
    SingleValueSerializer<Value> serializer;
    trie::Build<Writer, Key, SingleUint64Value, SingleValueSerializer<Value>>(
        tmpWriter, serializer, ukPostcodesKeyValuePairs);
  }

  rw_ops::Reverse(FileReader(tmpName), writer);

  header.m_trieSize = base::asserted_cast<uint32_t>(writer.Pos() - header.m_trieOffset);

  bytesWritten = writer.Pos();
  coding::WritePadding(writer, bytesWritten);

  header.m_pointsOffset = base::asserted_cast<uint32_t>(writer.Pos());

  {
    search::CentersTableBuilder builder;

    builder.SetGeometryParams(feature::DataHeader(container).GetBounds());
    for (size_t i = 0; i < valueMapping.size(); ++i)
      builder.Put(base::asserted_cast<uint32_t>(i), valueMapping[i]);

    builder.Freeze(writer);
  }

  header.m_pointsSize = base::asserted_cast<uint32_t>(writer.Pos() - header.m_pointsOffset);
  auto const endOffset = writer.Pos();
  writer.Seek(0);
  header.Serialize(writer);
  writer.Seek(endOffset);
  return true;
}

void BuildSearchIndex(FilesContainerR & container, vector<feature::AddressData> const & addrs,
                      Writer & indexWriter)
{
  using Key = strings::UniString;
  using Value = Uint64IndexValue;

  LOG(LINFO, ("Start building search index for", container.GetFileName()));
  base::Timer timer;

  auto const & categoriesHolder = GetDefaultCategories();

  FeaturesVectorTest features(container);
  SingleValueSerializer<Value> serializer;

  vector<pair<Key, Value>> searchIndexKeyValuePairs;
  AddFeatureNameIndexPairs(features, addrs, categoriesHolder, searchIndexKeyValuePairs);

  sort(searchIndexKeyValuePairs.begin(), searchIndexKeyValuePairs.end());
  LOG(LINFO, ("End sorting strings:", timer.ElapsedSeconds()));

  trie::Build<Writer, Key, ValueList<Value>, SingleValueSerializer<Value>>(
      indexWriter, serializer, searchIndexKeyValuePairs);

  LOG(LINFO, ("End building search index, elapsed seconds:", timer.ElapsedSeconds()));
}
}  // namespace indexer
