#include "retrieval.hpp"

#include "search/cancel_exception.hpp"
#include "search/feature_offset_match.hpp"
#include "search/interval_set.hpp"
#include "search/mwm_context.hpp"
#include "search/search_index_values.hpp"
#include "search/search_trie.hpp"
#include "search/token_slice.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_data.hpp"
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

using namespace strings;
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

  template <typename Value>
  void operator()(Value const & value)
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

  template <typename Fn>
  void ForEachModifiedOrCreated(Fn && fn)
  {
    ForEach(m_modified, fn);
    ForEach(m_created, fn);
  }

private:
  template <typename Fn>
  void ForEach(vector<uint32_t> const & features, Fn & fn)
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

template <typename DFA>
bool MatchesByName(vector<UniString> const & tokens, vector<DFA> const & dfas)
{
  for (auto const & dfa : dfas)
  {
    for (auto const & token : tokens)
    {
      auto it = dfa.Begin();
      DFAMove(it, token);
      if (it.Accepts())
        return true;
    }
  }

  return false;
}

template <typename DFA>
bool MatchesByType(feature::TypesHolder const & types, vector<DFA> const & dfas)
{
  if (dfas.empty())
    return false;

  auto const & c = classif();

  for (auto const & type : types)
  {
    UniString const s = FeatureTypeToString(c.GetIndexForType(type));

    for (auto const & dfa : dfas)
    {
      auto it = dfa.Begin();
      DFAMove(it, s);
      if (it.Accepts())
        return true;
    }
  }

  return false;
}

template <typename DFA>
bool MatchFeatureByNameAndType(FeatureType const & ft, SearchTrieRequest<DFA> const & request)
{
  feature::TypesHolder th(ft);

  bool matched = false;
  ft.ForEachName([&](int8_t lang, string const & name)
  {
    if (name.empty() || !request.IsLangExist(lang))
      return true /* continue ForEachName */;

    vector<UniString> tokens;
    NormalizeAndTokenizeString(name, tokens, Delimiters());
    if (!MatchesByName(tokens, request.m_names) && !MatchesByType(th, request.m_categories))
      return true /* continue ForEachName */;

    matched = true;
    return false /* break ForEachName */;
  });

  return matched;
}

bool MatchFeatureByPostcode(FeatureType const & ft, TokenSlice const & slice)
{
  string const postcode = ft.GetMetadata().Get(feature::Metadata::FMD_POSTCODE);
  vector<UniString> tokens;
  NormalizeAndTokenizeString(postcode, tokens, Delimiters());
  if (slice.Size() > tokens.size())
    return false;
  for (size_t i = 0; i < slice.Size(); ++i)
  {
    if (slice.IsPrefix(i))
    {
      if (!StartsWith(tokens[i], slice.Get(i).m_original))
        return false;
    }
    else if (tokens[i] != slice.Get(i).m_original)
    {
      return false;
    }
  }
  return true;
}

template<typename Value>
using TrieRoot = trie::Iterator<ValueList<Value>>;

template <typename Value, typename Fn>
void WithSearchTrieRoot(MwmValue & value, Fn && fn)
{
  serial::CodingParams codingParams(trie::GetCodingParams(value.GetHeader().GetDefCodingParams()));
  ModelReaderPtr searchReader = value.m_cont.GetReader(SEARCH_INDEX_FILE_TAG);

  auto const trieRoot = trie::ReadTrie<SubReaderWrapper<Reader>, ValueList<Value>>(
      SubReaderWrapper<Reader>(searchReader.GetPtr()), SingleValueSerializer<Value>(codingParams));

  return fn(*trieRoot);
}

// Retrieves from the search index corresponding to |value| all
// features matching to |params|.
template <typename Value, typename DFA>
unique_ptr<coding::CompressedBitVector> RetrieveAddressFeaturesImpl(
    MwmContext const & context, my::Cancellable const & cancellable,
    SearchTrieRequest<DFA> const & request)
{
  EditedFeaturesHolder holder(context.GetId());
  vector<uint64_t> features;
  FeaturesCollector collector(cancellable, features);

  WithSearchTrieRoot<Value>(context.m_value, [&](TrieRoot<Value> const & root) {
    MatchFeaturesInTrie(
        request, root,
        [&holder](uint32_t featureIndex) { return !holder.ModifiedOrDeleted(featureIndex); },
        collector);
  });

  holder.ForEachModifiedOrCreated([&](FeatureType & ft, uint64_t index) {
    if (MatchFeatureByNameAndType(ft, request))
      features.push_back(index);
  });

  return SortFeaturesAndBuildCBV(move(features));
}

template <typename Value>
unique_ptr<coding::CompressedBitVector> RetrievePostcodeFeaturesImpl(
    MwmContext const & context, my::Cancellable const & cancellable, TokenSlice const & slice)
{
  EditedFeaturesHolder holder(context.GetId());
  vector<uint64_t> features;
  FeaturesCollector collector(cancellable, features);

  WithSearchTrieRoot<Value>(context.m_value, [&](TrieRoot<Value> const & root) {
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
    MwmContext const & context, my::Cancellable const & cancellable, m2::RectD const & rect,
    int scale)
{
  EditedFeaturesHolder holder(context.GetId());

  covering::IntervalsT coverage;
  CoverRect(rect, scale, coverage);

  vector<uint64_t> features;

  FeaturesCollector collector(cancellable, features);

  context.ForEachIndex(coverage, scale, collector);

  holder.ForEachModifiedOrCreated([&](FeatureType & ft, uint64_t index) {
    auto const center = feature::GetCenter(ft);
    if (rect.IsPointInside(center))
      features.push_back(index);
  });
  return SortFeaturesAndBuildCBV(move(features));
}

template <typename T>
struct RetrieveAddressFeaturesAdaptor
{
  template <typename... Args>
  unique_ptr<coding::CompressedBitVector> operator()(Args &&... args)
  {
    return RetrieveAddressFeaturesImpl<T>(forward<Args>(args)...);
  }
};

template <typename T>
struct RetrievePostcodeFeaturesAdaptor
{
  template <typename... Args>
  unique_ptr<coding::CompressedBitVector> operator()(Args &&... args)
  {
    return RetrievePostcodeFeaturesImpl<T>(forward<Args>(args)...);
  }
};

template <template <typename> class T>
struct Selector
{
  template <typename... Args>
  unique_ptr<coding::CompressedBitVector> operator()(MwmContext const & context, Args &&... args)
  {
    version::MwmTraits mwmTraits(context.m_value.GetMwmVersion());

    if (mwmTraits.GetSearchIndexFormat() ==
        version::MwmTraits::SearchIndexFormat::FeaturesWithRankAndCenter)
    {
      T<FeatureWithRankAndCenter> t;
      return t(context, forward<Args>(args)...);
    }
    if (mwmTraits.GetSearchIndexFormat() ==
        version::MwmTraits::SearchIndexFormat::CompressedBitVector)
    {
      T<FeatureIndexValue> t;
      return t(context, forward<Args>(args)...);
    }
    return unique_ptr<coding::CompressedBitVector>();
  }
};
}  // namespace

unique_ptr<coding::CompressedBitVector> RetrieveAddressFeatures(
    MwmContext const & context, my::Cancellable const & cancellable,
    SearchTrieRequest<LevenshteinDFA> const & request)
{
  Selector<RetrieveAddressFeaturesAdaptor> selector;
  return selector(context, cancellable, request);
}

unique_ptr<coding::CompressedBitVector> RetrieveAddressFeatures(
    MwmContext const & context, my::Cancellable const & cancellable,
    SearchTrieRequest<PrefixDFAModifier<LevenshteinDFA>> const & request)
{
  Selector<RetrieveAddressFeaturesAdaptor> selector;
  return selector(context, cancellable, request);
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
  return RetrieveGeometryFeaturesImpl(context, cancellable, rect, scale);
}
} // namespace search
