#include "retrieval.hpp"

#include "search/cancel_exception.hpp"
#include "search/feature_offset_match.hpp"
#include "search/interval_set.hpp"
#include "search/mwm_context.hpp"
#include "search/search_index_values.hpp"
#include "search/search_trie.hpp"
#include "search/token_slice.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/index.hpp"
#include "indexer/osm_editor.hpp"
#include "indexer/scales.hpp"
#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"
#include "indexer/trie_reader.hpp"

#include "platform/mwm_traits.hpp"
#include "platform/mwm_version.hpp"

#include "coding/compressed_bit_vector.hpp"
#include "coding/reader_wrapper.hpp"

#include "std/algorithm.hpp"

using osm::Editor;

namespace search
{
namespace
{
class FeaturesCollector
{
public:
  FeaturesCollector(my::Cancellable const & cancellable, vector<uint64_t> & features)
    : m_cancellable(cancellable), m_features(features), m_counter(0)
  {
  }

  template <typename TValue>
  void operator()(TValue const & value)
  {
    if ((++m_counter & 0xFF) == 0)
      BailIfCancelled(m_cancellable);
    m_features.push_back(value.m_featureId);
  }

  inline void operator()(uint32_t feature) { m_features.push_back(feature); }

  inline void operator()(uint64_t feature) { m_features.push_back(feature); }

private:
  my::Cancellable const & m_cancellable;
  vector<uint64_t> & m_features;
  uint32_t m_counter;
};

class EditedFeaturesHolder
{
public:
  EditedFeaturesHolder(MwmSet::MwmId const & id) : m_id(id)
  {
    auto & editor = Editor::Instance();
    m_deleted = editor.GetFeaturesByStatus(id, Editor::FeatureStatus::Deleted);
    m_modified = editor.GetFeaturesByStatus(id, Editor::FeatureStatus::Modified);
    m_created = editor.GetFeaturesByStatus(id, Editor::FeatureStatus::Created);
  }

  bool ModifiedOrDeleted(uint32_t featureIndex) const
  {
    return binary_search(m_deleted.begin(), m_deleted.end(), featureIndex) ||
           binary_search(m_modified.begin(), m_modified.end(), featureIndex);
  }

  template <typename TFn>
  void ForEachModifiedOrCreated(TFn && fn)
  {
    ForEach(m_modified, fn);
    ForEach(m_created, fn);
  }

private:
  template <typename TFn>
  void ForEach(vector<uint32_t> const & features, TFn & fn)
  {
    auto & editor = Editor::Instance();
    for (auto const index : features)
    {
      FeatureType ft;
      VERIFY(editor.GetEditedFeature(m_id, index, ft), ());
      fn(ft, index);
    }
  }

  MwmSet::MwmId const & m_id;
  vector<uint32_t> m_deleted;
  vector<uint32_t> m_modified;
  vector<uint32_t> m_created;
};

unique_ptr<coding::CompressedBitVector> SortFeaturesAndBuildCBV(vector<uint64_t> && features)
{
  my::SortUnique(features);
  return coding::CompressedBitVectorBuilder::FromBitPositions(move(features));
}

/// Check that any from first matches any from second.
template <class TComp, class T>
bool IsFirstMatchesSecond(vector<T> const & first, vector<T> const & second, TComp const & comp)
{
  if (second.empty())
    return true;

  for (auto const & s : second)
  {
    for (auto const & f : first)
    {
      if (comp(f, s))
        return true;
    }
  }
  return false;
}

bool MatchFeatureByName(FeatureType const & ft, QueryParams const & params)
{
  using namespace strings;

  bool matched = false;
  ft.ForEachName([&](int8_t lang, string const & utf8Name)
  {
    if (utf8Name.empty() || params.m_langs.count(lang) == 0)
      return true;

    vector<UniString> nameTokens;
    NormalizeAndTokenizeString(utf8Name, nameTokens, Delimiters());

    auto const matchPrefix = [](UniString const & s1, UniString const & s2)
    {
      return StartsWith(s1, s2);
    };
    if (!IsFirstMatchesSecond(nameTokens, params.m_prefixTokens, matchPrefix))
      return true;

    for (auto const & synonyms : params.m_tokens)
    {
      if (!IsFirstMatchesSecond(nameTokens, synonyms, equal_to<UniString>()))
        return true;
    }

    matched = true;
    return false;
  });

  return matched;
}

bool MatchFeatureByPostcode(FeatureType const & ft, TokenSlice const & slice)
{
  string const postcode = ft.GetMetadata().Get(feature::Metadata::FMD_POSTCODE);
  vector<strings::UniString> tokens;
  NormalizeAndTokenizeString(postcode, tokens, Delimiters());
  if (slice.Size() > tokens.size())
    return false;
  for (size_t i = 0; i < slice.Size(); ++i)
  {
    if (slice.IsPrefix(i))
    {
      if (!StartsWith(tokens[i], slice.Get(i).front()))
        return false;
    }
    else if (tokens[i] != slice.Get(i).front())
    {
      return false;
    }
  }
  return true;
}

template<typename TValue>
using TrieRoot = trie::Iterator<ValueList<TValue>>;

template <typename TValue, typename TFn>
void WithSearchTrieRoot(MwmValue & value, TFn && fn)
{
  serial::CodingParams codingParams(trie::GetCodingParams(value.GetHeader().GetDefCodingParams()));
  ModelReaderPtr searchReader = value.m_cont.GetReader(SEARCH_INDEX_FILE_TAG);

  auto const trieRoot = trie::ReadTrie<SubReaderWrapper<Reader>, ValueList<TValue>>(
      SubReaderWrapper<Reader>(searchReader.GetPtr()), SingleValueSerializer<TValue>(codingParams));

  return fn(*trieRoot);
}

// Retrieves from the search index corresponding to |value| all
// features matching to |params|.
template <typename TValue>
unique_ptr<coding::CompressedBitVector> RetrieveAddressFeaturesImpl(
    MwmContext const & context, my::Cancellable const & cancellable, QueryParams const & params)
{
  EditedFeaturesHolder holder(context.GetId());
  vector<uint64_t> features;
  FeaturesCollector collector(cancellable, features);

  WithSearchTrieRoot<TValue>(context.m_value, [&](TrieRoot<TValue> const & root) {
    MatchFeaturesInTrie(
        params, root,
        [&holder](uint32_t featureIndex) { return !holder.ModifiedOrDeleted(featureIndex); },
        collector);
  });
  holder.ForEachModifiedOrCreated([&](FeatureType & ft, uint64_t index) {
    if (MatchFeatureByName(ft, params))
      features.push_back(index);
  });
  return SortFeaturesAndBuildCBV(move(features));
}

template <typename TValue>
unique_ptr<coding::CompressedBitVector> RetrievePostcodeFeaturesImpl(
    MwmContext const & context, my::Cancellable const & cancellable, TokenSlice const & slice)
{
  EditedFeaturesHolder holder(context.GetId());
  vector<uint64_t> features;
  FeaturesCollector collector(cancellable, features);

  WithSearchTrieRoot<TValue>(context.m_value, [&](TrieRoot<TValue> const & root) {
    MatchPostcodesInTrie(
        slice, root,
        [&holder](uint32_t featureIndex) { return !holder.ModifiedOrDeleted(featureIndex); },
        collector);
  });
  holder.ForEachModifiedOrCreated([&](FeatureType & ft, uint64_t index) {
    if (MatchFeatureByPostcode(ft, slice))
      features.push_back(index);
  });
  return SortFeaturesAndBuildCBV(move(features));
}

// Retrieves from the geometry index corresponding to handle all
// features from |coverage|.
unique_ptr<coding::CompressedBitVector> RetrieveGeometryFeaturesImpl(
    MwmContext const & context, my::Cancellable const & cancellable,
    covering::IntervalsT const & coverage, int scale)
{
  vector<uint64_t> features;

  FeaturesCollector collector(cancellable, features);

  context.ForEachIndex(coverage, scale, collector);
  return SortFeaturesAndBuildCBV(move(features));
}

template <typename T>
struct RetrieveAddressFeaturesAdaptor
{
  template <typename... TArgs>
  unique_ptr<coding::CompressedBitVector> operator()(TArgs &&... args)
  {
    return RetrieveAddressFeaturesImpl<T>(forward<TArgs>(args)...);
  }
};

template <typename T>
struct RetrievePostcodeFeaturesAdaptor
{
  template <typename... TArgs>
  unique_ptr<coding::CompressedBitVector> operator()(TArgs &&... args)
  {
    return RetrievePostcodeFeaturesImpl<T>(forward<TArgs>(args)...);
  }
};

template <template <typename> class T>
struct Selector
{
  template <typename... TArgs>
  unique_ptr<coding::CompressedBitVector> operator()(MwmContext const & context, TArgs &&... args)
  {
    version::MwmTraits mwmTraits(context.m_value.GetMwmVersion().GetFormat());

    if (mwmTraits.GetSearchIndexFormat() ==
        version::MwmTraits::SearchIndexFormat::FeaturesWithRankAndCenter)
    {
      T<FeatureWithRankAndCenter> t;
      return t(context, forward<TArgs>(args)...);
    }
    if (mwmTraits.GetSearchIndexFormat() ==
        version::MwmTraits::SearchIndexFormat::CompressedBitVector)
    {
      T<FeatureIndexValue> t;
      return t(context, forward<TArgs>(args)...);
    }
    return unique_ptr<coding::CompressedBitVector>();
  }
};
}  // namespace

unique_ptr<coding::CompressedBitVector> RetrieveAddressFeatures(MwmContext const & context,
                                                                my::Cancellable const & cancellable,
                                                                QueryParams const & params)
{
  Selector<RetrieveAddressFeaturesAdaptor> selector;
  return selector(context, cancellable, params);
}

unique_ptr<coding::CompressedBitVector> RetrievePostcodeFeatures(
    MwmContext const & context, my::Cancellable const & cancellable, TokenSlice const & slice)
{
  Selector<RetrievePostcodeFeaturesAdaptor> selector;
  return selector(context, cancellable, slice);
}

unique_ptr<coding::CompressedBitVector> RetrieveGeometryFeatures(
    MwmContext const & context, my::Cancellable const & cancellable, m2::RectD const & rect,
    int scale)
{
  covering::IntervalsT coverage;
  CoverRect(rect, scale, coverage);
  return RetrieveGeometryFeaturesImpl(context, cancellable, coverage, scale);
}
} // namespace search
