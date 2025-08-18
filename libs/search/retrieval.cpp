#include "search/retrieval.hpp"

#include "search/cancel_exception.hpp"
#include "search/feature_offset_match.hpp"
#include "search/mwm_context.hpp"
#include "search/search_index_header.hpp"
#include "search/search_index_values.hpp"
#include "search/token_slice.hpp"

#include "editor/osm_editor.hpp"

#include "indexer/classificator.hpp"
#include "indexer/editable_map_object.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_source.hpp"
#include "indexer/search_string_utils.hpp"
#include "indexer/trie_reader.hpp"

#include "platform/mwm_version.hpp"

#include "coding/compressed_bit_vector.hpp"
#include "coding/reader_wrapper.hpp"

#include "base/checked_cast.hpp"
#include "base/control_flow.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace search
{
using namespace std;
using namespace strings;
using osm::EditableMapObject;
using osm::Editor;

namespace
{
class FeaturesCollector
{
public:
  FeaturesCollector(base::Cancellable const & cancellable, vector<uint64_t> & features,
                    vector<uint64_t> & exactlyMatchedFeatures)
    : m_cancellable(cancellable)
    , m_features(features)
    , m_exactlyMatchedFeatures(exactlyMatchedFeatures)
    , m_counter(0)
  {}

  template <typename Value>
  void operator()(Value const & value, bool exactMatch)
  {
    if ((++m_counter & 0xFF) == 0)
      BailIfCancelled(m_cancellable);

    m_features.emplace_back(value.m_featureId);
    if (exactMatch)
      m_exactlyMatchedFeatures.emplace_back(value.m_featureId);
  }

  void operator()(uint32_t feature)
  {
    if ((++m_counter & 0xFF) == 0)
      BailIfCancelled(m_cancellable);

    m_features.emplace_back(feature);
  }

  void operator()(uint64_t feature, bool exactMatch)
  {
    if ((++m_counter & 0xFF) == 0)
      BailIfCancelled(m_cancellable);

    m_features.emplace_back(feature);
    if (exactMatch)
      m_exactlyMatchedFeatures.emplace_back(feature);
  }

private:
  base::Cancellable const & m_cancellable;
  vector<uint64_t> & m_features;
  vector<uint64_t> & m_exactlyMatchedFeatures;
  uint32_t m_counter;
};

class EditedFeaturesHolder
{
public:
  explicit EditedFeaturesHolder(MwmSet::MwmId const & id) : m_id(id)
  {
    auto & editor = Editor::Instance();
    m_deleted = editor.GetFeaturesByStatus(id, FeatureStatus::Deleted);
    m_modified = editor.GetFeaturesByStatus(id, FeatureStatus::Modified);
    m_created = editor.GetFeaturesByStatus(id, FeatureStatus::Created);
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
      // Ignore feature load errors related to mwm removal and feature parse errors from editor.
      if (auto emo = editor.GetEditedFeature(FeatureID(m_id, index)))
        fn(*emo, index);
    }
  }

  MwmSet::MwmId const & m_id;
  vector<uint32_t> m_deleted;
  vector<uint32_t> m_modified;
  vector<uint32_t> m_created;
};

Retrieval::ExtendedFeatures SortFeaturesAndBuildResult(vector<uint64_t> && features,
                                                       vector<uint64_t> && exactlyMatchedFeatures)
{
  using Builder = coding::CompressedBitVectorBuilder;
  base::SortUnique(features);
  base::SortUnique(exactlyMatchedFeatures);
  auto featuresCBV = CBV(Builder::FromBitPositions(std::move(features)));
  auto exactlyMatchedFeaturesCBV = CBV(Builder::FromBitPositions(std::move(exactlyMatchedFeatures)));
  return Retrieval::ExtendedFeatures(std::move(featuresCBV), std::move(exactlyMatchedFeaturesCBV));
}

Retrieval::ExtendedFeatures SortFeaturesAndBuildResult(vector<uint64_t> && features)
{
  using Builder = coding::CompressedBitVectorBuilder;
  base::SortUnique(features);
  auto const featuresCBV = CBV(Builder::FromBitPositions(std::move(features)));
  return Retrieval::ExtendedFeatures(featuresCBV);
}

template <typename DFA>
pair<bool, bool> MatchesByName(vector<UniString> const & tokens, vector<DFA> const & dfas)
{
  for (auto const & dfa : dfas)
  {
    for (auto const & token : tokens)
    {
      auto it = dfa.Begin();
      DFAMove(it, token);
      if (it.Accepts())
        return {true, it.ErrorsMade() == 0};
    }
  }

  return {false, false};
}

template <typename DFA>
pair<bool, bool> MatchesByType(feature::TypesHolder const & types, vector<DFA> const & dfas)
{
  if (dfas.empty())
    return {false, false};

  auto const & c = classif();

  for (auto const & type : types)
  {
    UniString const s = FeatureTypeToString(c.GetIndexForType(type));

    for (auto const & dfa : dfas)
    {
      auto it = dfa.Begin();
      DFAMove(it, s);
      if (it.Accepts())
        return {true, it.ErrorsMade() == 0};
    }
  }

  return {false, false};
}

template <typename DFA>
pair<bool, bool> MatchFeatureByNameAndType(EditableMapObject const & emo, SearchTrieRequest<DFA> const & request)
{
  auto const & th = emo.GetTypes();

  pair<bool, bool> matchedByType = MatchesByType(th, request.m_categories);

  // Exactly matched by type.
  if (matchedByType.second)
    return {true, true};

  pair<bool, bool> matchedByName = {false, false};
  emo.GetNameMultilang().ForEach([&](int8_t lang, string_view name)
  {
    if (name.empty() || !request.HasLang(lang))
      return base::ControlFlow::Continue;

    auto const tokens = NormalizeAndTokenizeString(name);
    auto const matched = MatchesByName(tokens, request.m_names);
    matchedByName = {matchedByName.first || matched.first, matchedByName.second || matched.second};
    if (!matchedByName.second)
      return base::ControlFlow::Continue;

    return base::ControlFlow::Break;
  });

  return {matchedByType.first || matchedByName.first, matchedByType.second || matchedByName.second};
}

bool MatchFeatureByPostcode(EditableMapObject const & emo, TokenSlice const & slice)
{
  auto const tokens = NormalizeAndTokenizeString(emo.GetPostcode());
  if (slice.Size() > tokens.size())
    return false;
  for (size_t i = 0; i < slice.Size(); ++i)
  {
    if (slice.IsPrefix(i))
    {
      if (!StartsWith(tokens[i], slice.Get(i).GetOriginal()))
        return false;
    }
    else if (tokens[i] != slice.Get(i).GetOriginal())
    {
      return false;
    }
  }
  return true;
}

template <typename Value, typename DFA>
Retrieval::ExtendedFeatures RetrieveAddressFeaturesImpl(Retrieval::TrieRoot<Value> const & root,
                                                        MwmContext const & context,
                                                        base::Cancellable const & cancellable,
                                                        SearchTrieRequest<DFA> const & request)
{
  EditedFeaturesHolder holder(context.GetId());
  vector<uint64_t> features;
  vector<uint64_t> exactlyMatchedFeatures;
  FeaturesCollector collector(cancellable, features, exactlyMatchedFeatures);

  MatchFeaturesInTrie(request, root, [&holder](Value const & value)
  { return !holder.ModifiedOrDeleted(base::asserted_cast<uint32_t>(value.m_featureId)); } /* filter */, collector);

  holder.ForEachModifiedOrCreated([&](EditableMapObject const & emo, uint64_t index)
  {
    auto const matched = MatchFeatureByNameAndType(emo, request);
    if (matched.first)
    {
      features.emplace_back(index);
      if (matched.second)
        exactlyMatchedFeatures.emplace_back(index);
    }
  });

  return SortFeaturesAndBuildResult(std::move(features), std::move(exactlyMatchedFeatures));
}

template <typename Value>
Retrieval::ExtendedFeatures RetrievePostcodeFeaturesImpl(Retrieval::TrieRoot<Value> const & root,
                                                         MwmContext const & context,
                                                         base::Cancellable const & cancellable,
                                                         TokenSlice const & slice)
{
  EditedFeaturesHolder holder(context.GetId());
  vector<uint64_t> features;
  vector<uint64_t> exactlyMatchedFeatures;
  FeaturesCollector collector(cancellable, features, exactlyMatchedFeatures);

  MatchPostcodesInTrie(slice, root, [&holder](Value const & value)
  { return !holder.ModifiedOrDeleted(base::asserted_cast<uint32_t>(value.m_featureId)); } /* filter */, collector);

  holder.ForEachModifiedOrCreated([&](EditableMapObject const & emo, uint64_t index)
  {
    if (MatchFeatureByPostcode(emo, slice))
      features.push_back(index);
  });

  return SortFeaturesAndBuildResult(std::move(features));
}

Retrieval::ExtendedFeatures RetrieveGeometryFeaturesImpl(MwmContext const & context,
                                                         base::Cancellable const & cancellable, m2::RectD const & rect,
                                                         int scale)
{
  EditedFeaturesHolder holder(context.GetId());

  covering::Intervals coverage;
  CoverRect(rect, scale, coverage);

  vector<uint64_t> features;
  vector<uint64_t> exactlyMatchedFeatures;

  FeaturesCollector collector(cancellable, features, exactlyMatchedFeatures);

  context.ForEachIndex(coverage, scale, collector);

  holder.ForEachModifiedOrCreated([&](EditableMapObject const & emo, uint64_t index)
  {
    auto const center = emo.GetMercator();
    if (rect.IsPointInside(center))
      features.push_back(index);
  });
  return SortFeaturesAndBuildResult(std::move(features), std::move(exactlyMatchedFeatures));
}

template <typename T>
struct RetrieveAddressFeaturesAdaptor
{
  template <typename... Args>
  Retrieval::ExtendedFeatures operator()(Args &&... args)
  {
    return RetrieveAddressFeaturesImpl<T>(std::forward<Args>(args)...);
  }
};

template <typename T>
struct RetrievePostcodeFeaturesAdaptor
{
  template <typename... Args>
  Retrieval::ExtendedFeatures operator()(Args &&... args)
  {
    return RetrievePostcodeFeaturesImpl<T>(std::forward<Args>(args)...);
  }
};

template <typename Value>
unique_ptr<Retrieval::TrieRoot<Value>> ReadTrie(ModelReaderPtr & reader)
{
  return trie::ReadTrie<SubReaderWrapper<Reader>, ValueList<Value>>(SubReaderWrapper<Reader>(reader.GetPtr()),
                                                                    SingleValueSerializer<Value>());
}
}  // namespace

Retrieval::Retrieval(MwmContext const & context, base::Cancellable const & cancellable)
  : m_context(context)
  , m_cancellable(cancellable)
  , m_reader(unique_ptr<ModelReader>())
{
  auto const & value = context.m_value;

  version::MwmTraits mwmTraits(value.GetMwmVersion());
  auto const format = mwmTraits.GetSearchIndexFormat();
  if (format == version::MwmTraits::SearchIndexFormat::CompressedBitVector)
  {
    m_reader = context.m_value.m_cont.GetReader(SEARCH_INDEX_FILE_TAG);
  }
  else if (format == version::MwmTraits::SearchIndexFormat::CompressedBitVectorWithHeader)
  {
    FilesContainerR::TReader reader = value.m_cont.GetReader(SEARCH_INDEX_FILE_TAG);

    SearchIndexHeader header;
    header.Read(*reader.GetPtr());
    CHECK(header.m_version == SearchIndexHeader::Version::V2, (base::Underlying(header.m_version)));

    m_reader = reader.SubReader(header.m_indexOffset, header.m_indexSize);
  }
  else
  {
    CHECK(false, ("Unsupported search index format", format));
  }
  m_root = ReadTrie<Uint64IndexValue>(m_reader);
}

Retrieval::ExtendedFeatures Retrieval::RetrieveAddressFeatures(SearchTrieRequest<UniStringDFA> const & request) const
{
  return Retrieve<RetrieveAddressFeaturesAdaptor>(request);
}

Retrieval::ExtendedFeatures Retrieval::RetrieveAddressFeatures(
    SearchTrieRequest<PrefixDFAModifier<UniStringDFA>> const & request) const
{
  return Retrieve<RetrieveAddressFeaturesAdaptor>(request);
}

Retrieval::ExtendedFeatures Retrieval::RetrieveAddressFeatures(SearchTrieRequest<LevenshteinDFA> const & request) const
{
  return Retrieve<RetrieveAddressFeaturesAdaptor>(request);
}

Retrieval::ExtendedFeatures Retrieval::RetrieveAddressFeatures(
    SearchTrieRequest<PrefixDFAModifier<LevenshteinDFA>> const & request) const
{
  return Retrieve<RetrieveAddressFeaturesAdaptor>(request);
}

Retrieval::Features Retrieval::RetrievePostcodeFeatures(TokenSlice const & slice) const
{
  return Retrieve<RetrievePostcodeFeaturesAdaptor>(slice).m_features;
}

Retrieval::Features Retrieval::RetrieveGeometryFeatures(m2::RectD const & rect, int scale) const
{
  return RetrieveGeometryFeaturesImpl(m_context, m_cancellable, rect, scale).m_features;
}

template <template <typename> class R, typename... Args>
Retrieval::ExtendedFeatures Retrieval::Retrieve(Args &&... args) const
{
  R<Uint64IndexValue> r;
  ASSERT(m_root, ());
  return r(*m_root, m_context, m_cancellable, std::forward<Args>(args)...);
}
}  // namespace search
