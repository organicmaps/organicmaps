#include "search_index_builder.hpp"

#include "search/common.hpp"
#include "search/mwm_context.hpp"
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
#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"
#include "indexer/trie_builder.hpp"

#include "defines.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/fixed_bits_ddvector.hpp"
#include "coding/reader_writer_ops.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
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

using namespace std;

#define SYNONYMS_FILE "synonyms.txt"

namespace
{
class SynonymsHolder
{
  unordered_multimap<string, string> m_map;

public:
  SynonymsHolder(string const & fPath)
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
          // synonym should not has any spaces
          ASSERT ( tokens[i].find_first_of(" \t") == string::npos, () );
          m_map.insert(make_pair(tokens[0], tokens[i]));
        }
      }
    }
  }

  template <class ToDo>
  void ForEach(string const & key, ToDo toDo) const
  {
    using TIter = unordered_multimap<string, string>::const_iterator;

    pair<TIter, TIter> range = m_map.equal_range(key);
    while (range.first != range.second)
    {
      toDo(range.first->second);
      ++range.first;
    }
  }
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

template <typename TKey, typename TValue>
struct FeatureNameInserter
{
  SynonymsHolder * m_synonyms;
  vector<pair<TKey, TValue>> & m_keyValuePairs;
  TValue m_val;

  bool m_hasStreetType;

  FeatureNameInserter(SynonymsHolder * synonyms, vector<pair<TKey, TValue>> & keyValuePairs,
                      bool hasStreetType)
    : m_synonyms(synonyms), m_keyValuePairs(keyValuePairs), m_hasStreetType(hasStreetType)
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
      search::StreetTokensFilter filter([&](strings::UniString const & token, size_t /* tag */)
                                        {
                                          AddToken(lang, token);
                                        });
      for (auto const & token : tokens)
        filter.Put(token, false /* isPrefix */, 0 /* tag */);
    }
    else
    {
      for (auto const & token : tokens)
        AddToken(lang, token);
    }
  }
};

template <typename TValue>
struct ValueBuilder;

template <>
struct ValueBuilder<FeatureWithRankAndCenter>
{
  ValueBuilder() = default;

  void MakeValue(FeatureType & ft, uint32_t index, FeatureWithRankAndCenter & v) const
  {
    v.m_featureId = index;

    // get BEST geometry rect of feature
    v.m_pt = feature::GetCenter(ft);
    v.m_rank = feature::PopulationToRank(ft.GetPopulation());
  }
};

template <>
struct ValueBuilder<FeatureIndexValue>
{
  ValueBuilder() = default;

  void MakeValue(FeatureType const & /* f */, uint32_t index, FeatureIndexValue & value) const
  {
    value.m_featureId = index;
  }
};

template <typename TKey, typename TValue>
class FeatureInserter
{
  SynonymsHolder * m_synonyms;
  vector<pair<TKey, TValue>> & m_keyValuePairs;

  CategoriesHolder const & m_categories;

  pair<int, int> m_scales;

  ValueBuilder<TValue> const & m_valueBuilder;

public:
  FeatureInserter(SynonymsHolder * synonyms, vector<pair<TKey, TValue>> & keyValuePairs,
                  CategoriesHolder const & catHolder, pair<int, int> const & scales,
                  ValueBuilder<TValue> const & valueBuilder)
    : m_synonyms(synonyms)
    , m_keyValuePairs(keyValuePairs)
    , m_categories(catHolder)
    , m_scales(scales)
    , m_valueBuilder(valueBuilder)
  {
  }

  void operator()(FeatureType & f, uint32_t index) const
  {
    using namespace search;

    static TypesSkipper skipIndex;

    feature::TypesHolder types(f);

    auto const & streetChecker = ftypes::IsStreetChecker::Instance();
    bool const hasStreetType = streetChecker(types);

    // Init inserter with serialized value.
    // Insert synonyms only for countries and states (maybe will add cities in future).
    FeatureNameInserter<TKey, TValue> inserter(
        skipIndex.IsCountryOrState(types) ? m_synonyms : nullptr, m_keyValuePairs, hasStreetType);
    m_valueBuilder.MakeValue(f, index, inserter.m_val);

    string const postcode = f.GetMetadata().Get(feature::Metadata::FMD_POSTCODE);
    if (!postcode.empty())
    {
      // See OSM TagInfo or Wiki about modern postcodes format. The
      // mean number of tokens is less than two.
      buffer_vector<strings::UniString, 2> tokens;
      SplitUniString(NormalizeAndSimplifyString(postcode), base::MakeBackInsertFunctor(tokens),
                     Delimiters());
      for (auto const & token : tokens)
        inserter.AddToken(kPostcodesLang, token);
    }

    skipIndex.SkipTypes(types);
    if (types.Empty())
      return;

    // Skip types for features without names.
    if (!f.ForEachName(inserter))
      skipIndex.SkipEmptyNameTypes(types);
    if (types.Empty())
      return;

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
};

template <typename TKey, typename TValue>
void AddFeatureNameIndexPairs(FeaturesVectorTest const & features,
                              CategoriesHolder const & categoriesHolder,
                              vector<pair<TKey, TValue>> & keyValuePairs)
{
  feature::DataHeader const & header = features.GetHeader();

  ValueBuilder<TValue> valueBuilder;

  unique_ptr<SynonymsHolder> synonyms;
  if (header.GetType() == feature::DataHeader::world)
    synonyms.reset(new SynonymsHolder(GetPlatform().WritablePathForFile(SYNONYMS_FILE)));

  features.GetVector().ForEach(FeatureInserter<TKey, TValue>(
      synonyms.get(), keyValuePairs, categoriesHolder, header.GetScaleRange(), valueBuilder));
}

bool GetStreetIndex(search::MwmContext & ctx, uint32_t featureID, string const & streetName,
                    uint32_t & result)
{
  size_t streetIndex = 0;
  strings::UniString const street = search::GetStreetNameAsKey(streetName);

  bool const hasStreet = !street.empty();
  if (hasStreet)
  {
    FeatureType ft;
    VERIFY(ctx.GetFeature(featureID, ft), ());

    using TStreet = search::ReverseGeocoder::Street;
    vector<TStreet> streets;
    search::ReverseGeocoder::GetNearbyStreets(ctx, feature::GetCenter(ft), streets);

    streetIndex = search::ReverseGeocoder::GetMatchedStreetIndex(street, streets);
    if (streetIndex < streets.size())
    {
      result = base::checked_cast<uint32_t>(streetIndex);
      return true;
    }
  }

  result = hasStreet ? 1 : 0;
  return false;
}

void BuildAddressTable(FilesContainerR & container, Writer & writer, uint32_t threadsCount)
{
  // Read all street names to memory.
  ReaderSource<ModelReaderPtr> src(container.GetReader(SEARCH_TOKENS_FILE_TAG));
  vector<feature::AddressData> addrs;
  while (src.Size() > 0)
  {
    addrs.push_back({});
    addrs.back().Deserialize(src);
  }
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
  map<size_t, size_t> bounds;

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
      bool const found = GetStreetIndex(*(contexts[threadIdx]), i,
                                        addrs[i].Get(feature::AddressData::STREET), streetIndex);

      lock_guard<mutex> guard(resMutex);

      if (found)
      {
        results[i] = streetIndex;
        ++bounds[streetIndex];
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
    FixedBitsDDVector<3, FileReader>::Builder<Writer> building2Street(writer);
    for (auto i : results)
    {
      if (i == kEmptyResult)
        building2Street.PushBackUndefined();
      else
        building2Street.PushBack(i);
    }

    LOG(LINFO, ("Address: Building -> Street (opt, all)", building2Street.GetCount()));
  }

  double matchedPercent = 100;
  if (address > 0)
    matchedPercent = 100.0 * (1.0 - static_cast<double>(missing) / static_cast<double>(address));
  LOG(LINFO, ("Address: Matched percent", matchedPercent));
  LOG(LINFO, ("Address: Upper bounds", bounds));
}
}  // namespace

namespace indexer
{
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
    {
      FileWriter writer(indexFilePath);
      BuildSearchIndex(readContainer, writer);
      LOG(LINFO, ("Search index size =", writer.Size()));
    }
    if (filename != WORLD_FILE_NAME && filename != WORLD_COASTS_FILE_NAME)
    {
      FileWriter writer(addrFilePath);
      BuildAddressTable(readContainer, writer, threadsCount);
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
        writeContainer.DeleteSection(SEARCH_TOKENS_FILE_TAG);
      }

      // Separate scopes because FilesContainerW cannot write two sections at once.
      {
        FilesContainerW writeContainer(readContainer.GetFileName(), FileWriter::OP_WRITE_EXISTING);
        FileWriter writer = writeContainer.GetWriter(SEARCH_INDEX_FILE_TAG);
        rw_ops::Reverse(FileReader(indexFilePath), writer);
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
  using Value = FeatureIndexValue;

  LOG(LINFO, ("Start building search index for", container.GetFileName()));
  base::Timer timer;

  auto const & categoriesHolder = GetDefaultCategories();

  FeaturesVectorTest features(container);
  auto codingParams =
      trie::GetGeometryCodingParams(features.GetHeader().GetDefGeometryCodingParams());
  SingleValueSerializer<Value> serializer(codingParams);

  vector<pair<Key, Value>> searchIndexKeyValuePairs;
  AddFeatureNameIndexPairs(features, categoriesHolder, searchIndexKeyValuePairs);

  sort(searchIndexKeyValuePairs.begin(), searchIndexKeyValuePairs.end());
  LOG(LINFO, ("End sorting strings:", timer.ElapsedSeconds()));

  trie::Build<Writer, Key, ValueList<Value>, SingleValueSerializer<Value>>(
      indexWriter, serializer, searchIndexKeyValuePairs);

  LOG(LINFO, ("End building search index, elapsed seconds:", timer.ElapsedSeconds()));
}
}  // namespace indexer
