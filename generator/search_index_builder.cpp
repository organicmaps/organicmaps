#include "search_index_builder.hpp"

#include "search/common.hpp"
#include "search/reverse_geocoder.hpp"
#include "search/search_index_values.hpp"
#include "search/search_trie.hpp"
#include "search/types_skipper.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
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
#include "base/stl_add.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include <algorithm>
#include <fstream>
#include <initializer_list>
#include <limits>
#include <unordered_map>
#include <vector>

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
      strings::Tokenize(line, ":,", MakeBackInsertFunctor(tokens));

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
  Classificator const & c = classif();
  auto const & invisibleChecker = ftypes::IsInvisibleIndexedChecker::Instance();

  for (uint32_t t : types)
  {
    // Leave only 2 levels of types - for example, do not distinguish:
    // highway-primary-bridge and highway-primary-tunnel
    // or amenity-parking-fee and amenity-parking-underground-fee.
    ftype::TruncValue(t, 2);

    // Only categorized types will be added to index.
    if (!categories.IsTypeExist(t))
      continue;

    // There are some special non-drawable types we plan to search on.
    if (invisibleChecker.IsMatched(t))
    {
      result.push_back(t);
      continue;
    }

    // Index only those types that are visible.
    pair<int, int> r = feature::GetDrawableScaleRange(t);
    CHECK_LESS_OR_EQUAL(r.first, r.second, (c.GetReadableObjectName(t)));

    // Drawable scale must be normalized to indexer scales.
    r.second = min(r.second, scales::GetUpperScale());
    r.first = min(r.first, r.second);
    CHECK(r.first != -1, (c.GetReadableObjectName(t)));

    if (r.second >= scaleRange.first && r.first <= scaleRange.second)
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
    SplitUniString(uniName, MakeBackInsertFunctor(tokens), search::Delimiters());

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

  void MakeValue(FeatureType const & ft, uint32_t index, FeatureWithRankAndCenter & v) const
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

  void operator() (FeatureType const & f, uint32_t index) const
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
      SplitUniString(NormalizeAndSimplifyString(postcode), MakeBackInsertFunctor(tokens),
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
                              CategoriesHolder & categoriesHolder,
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

void BuildAddressTable(FilesContainerR & container, Writer & writer)
{
  ReaderSource<ModelReaderPtr> src = container.GetReader(SEARCH_TOKENS_FILE_TAG);
  uint32_t address = 0, missing = 0;
  map<size_t, size_t> bounds;

  Index mwmIndex;
  /// @ todo Make some better solution, or legalize MakeTemporary.
  auto const res = mwmIndex.RegisterMap(platform::LocalCountryFile::MakeTemporary(container.GetFileName()));
  ASSERT_EQUAL(res.second, MwmSet::RegResult::Success, ());
  search::ReverseGeocoder rgc(mwmIndex);

  {
    FixedBitsDDVector<3, FileReader>::Builder<Writer> building2Street(writer);

    FeaturesVectorTest features(container);
    for (uint32_t index = 0; src.Size() > 0; ++index)
    {
      feature::AddressData data;
      data.Deserialize(src);

      size_t streetIndex = 0;
      bool streetMatched = false;
      strings::UniString const street = search::GetStreetNameAsKey(data.Get(feature::AddressData::STREET));
      if (!street.empty())
      {
        FeatureType ft;
        features.GetVector().GetByIndex(index, ft);
        ft.SetID({res.first, index});

        using TStreet = search::ReverseGeocoder::Street;
        vector<TStreet> streets;
        rgc.GetNearbyStreets(ft, streets);

        streetIndex = rgc.GetMatchedStreetIndex(street, streets);
        if (streetIndex < streets.size())
        {
          ++bounds[streetIndex];
          streetMatched = true;
        }
        else
        {
          ++missing;
        }
        ++address;
      }
      if (streetMatched)
        building2Street.PushBack(base::checked_cast<decltype(building2Street)::ValueType>(streetIndex));
      else
        building2Street.PushBackUndefined();
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
bool BuildSearchIndexFromDataFile(string const & filename, bool forceRebuild)
{
  Platform & platform = GetPlatform();

  FilesContainerR readContainer(platform.GetReader(filename, "f"));
  if (readContainer.IsExist(SEARCH_INDEX_FILE_TAG) && !forceRebuild)
    return true;

  string mwmName = filename;
  my::GetNameFromFullPath(mwmName);
  my::GetNameWithoutExt(mwmName);

  string const indexFilePath = platform.WritablePathForFile(
        mwmName + "." SEARCH_INDEX_FILE_TAG EXTENSION_TMP);
  MY_SCOPE_GUARD(indexFileGuard, bind(&FileWriter::DeleteFileX, indexFilePath));

  string const addrFilePath = platform.WritablePathForFile(
        mwmName + "." SEARCH_ADDRESS_FILE_TAG EXTENSION_TMP);
  MY_SCOPE_GUARD(addrFileGuard, bind(&FileWriter::DeleteFileX, addrFilePath));

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
      BuildAddressTable(readContainer, writer);
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

  Platform & platform = GetPlatform();

  LOG(LINFO, ("Start building search index for", container.GetFileName()));
  my::Timer timer;

  CategoriesHolder categoriesHolder(platform.GetReader(SEARCH_CATEGORIES_FILE_NAME));

  FeaturesVectorTest features(container);
  auto codingParams = trie::GetCodingParams(features.GetHeader().GetDefCodingParams());
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
