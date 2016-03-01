#include "retrieval.hpp"

#include "cancel_exception.hpp"
#include "feature_offset_match.hpp"
#include "interval_set.hpp"
#include "search_index_values.hpp"
#include "search_trie.hpp"

#include "v2/mwm_context.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/index.hpp"
#include "indexer/osm_editor.hpp"
#include "indexer/scales.hpp"
#include "indexer/search_string_utils.hpp"
#include "indexer/trie_reader.hpp"

#include "platform/mwm_traits.hpp"
#include "platform/mwm_version.hpp"

#include "coding/compressed_bit_vector.hpp"
#include "coding/reader_wrapper.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include "std/algorithm.hpp"
#include "std/cmath.hpp"
#include "std/limits.hpp"

using osm::Editor;

namespace search
{
namespace
{
// A value used to represent maximum viewport scale.
double const kInfinity = numeric_limits<double>::infinity();

// Maximum viewport scale level when we stop to expand viewport and
// simply return all rest features from mwms.
double const kMaxViewportScaleLevel = scales::GetUpperCountryScale();

// Upper bound on a number of features when fast path is used.
// Otherwise, slow path is used.
uint64_t constexpr kFastPathThreshold = 100;

unique_ptr<coding::CompressedBitVector> SortFeaturesAndBuildCBV(vector<uint64_t> && features)
{
  my::SortUnique(features);
  return coding::CompressedBitVectorBuilder::FromBitPositions(move(features));
}

bool MatchFeatureByName(FeatureType const & ft, SearchQueryParams const & params)
{
  bool matched = false;
  auto const matcher = [&](int8_t /*lang*/, string const & utf8Name) -> bool
  {
    auto const name = search::NormalizeAndSimplifyString(utf8Name);
    for (auto const & prefix : params.m_prefixTokens)
    {
      if (strings::StartsWith(name, prefix))
      {
        matched = true;
        return false;
      }
    }
    for (auto const & synonyms : params.m_tokens)
    {
      for (auto const & token : synonyms)
      {
        if (token == name)
        {
          matched = true;
          return false;
        }
      }
    }
    return true;  // To iterate all names.
  };
  ft.ForEachName(matcher);
  return matched;
}

// Retrieves from the search index corresponding to |value| all
// features matching to |params|.
template <typename TValue>
unique_ptr<coding::CompressedBitVector> RetrieveAddressFeaturesImpl(
    MwmSet::MwmId const & id, MwmValue & value, my::Cancellable const & cancellable,
    SearchQueryParams const & params)
{
  serial::CodingParams codingParams(trie::GetCodingParams(value.GetHeader().GetDefCodingParams()));
  ModelReaderPtr searchReader = value.m_cont.GetReader(SEARCH_INDEX_FILE_TAG);

  // Exclude from search all deleted/modified features and match all edited/created features separately.
  Editor & editor = Editor::Instance();
  auto const deleted = editor.GetFeaturesByStatus(id, Editor::FeatureStatus::Deleted);
  // Modified features should be re-matched again.
  auto const modified = editor.GetFeaturesByStatus(id, Editor::FeatureStatus::Modified);
  auto const filter = [&](uint32_t featureIndex) -> bool
  {
    if (binary_search(deleted.begin(), deleted.end(), featureIndex))
      return false;
    if (binary_search(modified.begin(), modified.end(), featureIndex))
      return false;
    return true;
  };

  auto const trieRoot = trie::ReadTrie<SubReaderWrapper<Reader>, ValueList<TValue>>(
      SubReaderWrapper<Reader>(searchReader.GetPtr()), SingleValueSerializer<TValue>(codingParams));

  // TODO (@y, @m): This code may be optimized in the case where
  // bit vectors are sorted in the search index.
  vector<uint64_t> features;
  uint32_t counter = 0;
  auto const collector = [&](TValue const & value)
  {
    if ((++counter & 0xFF) == 0)
      BailIfCancelled(cancellable);
    features.push_back(value.m_featureId);
  };

  MatchFeaturesInTrie(params, *trieRoot, filter, collector);

  // Match all edited/created features separately.
  auto const matcher = [&](uint32_t featureIndex)
  {
    FeatureType ft;
    VERIFY(editor.GetEditedFeature(id, featureIndex, ft), ());
    // TODO(AlexZ): Should we match by some feature's metafields too?
    if (MatchFeatureByName(ft, params))
      features.push_back(featureIndex);
  };
  for_each(modified.begin(), modified.end(), matcher);
  auto const created = editor.GetFeaturesByStatus(id, Editor::FeatureStatus::Created);
  for_each(created.begin(), created.end(), matcher);

  return SortFeaturesAndBuildCBV(move(features));
}

// Retrieves from the geometry index corresponding to handle all
// features from |coverage|.
unique_ptr<coding::CompressedBitVector> RetrieveGeometryFeaturesImpl(
    v2::MwmContext const & context, my::Cancellable const & cancellable,
    covering::IntervalsT const & coverage, int scale)
{
  uint32_t counter = 0;
  vector<uint64_t> features;

  context.ForEachIndex(coverage, scale, [&](uint64_t featureId)
  {
    if ((++counter & 0xFF) == 0)
      BailIfCancelled(cancellable);
    features.push_back(featureId);
  });

  return SortFeaturesAndBuildCBV(move(features));
}

// This class represents a fast retrieval strategy.  When number of
// matching features in an mwm is small, it is worth computing their
// centers explicitly, by loading geometry from mwm.
class FastPathStrategy : public Retrieval::Strategy
{
public:
  FastPathStrategy(Index const & index, MwmSet::MwmHandle & handle, m2::RectD const & viewport,
                   unique_ptr<coding::CompressedBitVector> && addressFeatures)
    : Strategy(handle, viewport), m_lastReported(0)
  {
    ASSERT(addressFeatures.get(),
           ("Strategy must be initialized with valid address features set."));

    m2::PointD const center = m_viewport.Center();

    Index::FeaturesLoaderGuard loader(index, m_handle.GetId());
    coding::CompressedBitVectorEnumerator::ForEach(
        *addressFeatures, [&](uint64_t featureId)
        {
          ASSERT_LESS_OR_EQUAL(featureId, numeric_limits<uint32_t>::max(), ());
          FeatureType feature;
          loader.GetFeatureByIndex(featureId, feature);
          m_features.emplace_back(featureId,
                                  feature::GetCenter(feature, FeatureType::WORST_GEOMETRY));
        });

    // Order features by distance from the center of |viewport|.
    sort(m_features.begin(), m_features.end(),
         [&center](pair<uint32_t, m2::PointD> const & lhs, pair<uint32_t, m2::PointD> const & rhs)
         {
           return lhs.second.SquareLength(center) < rhs.second.SquareLength(center);
         });
  }

  // Retrieval::Strategy overrides:
  bool RetrieveImpl(double scale, my::Cancellable const & /* cancellable */,
                    TCallback const & callback) override
  {
    m2::RectD viewport = m_viewport;
    viewport.Scale(scale);

    vector<uint64_t> features;

    ASSERT_LESS_OR_EQUAL(m_lastReported, m_features.size(), ());
    while (m_lastReported < m_features.size() &&
           viewport.IsPointInside(m_features[m_lastReported].second))
    {
      features.push_back(m_features[m_lastReported].first);
      ++m_lastReported;
    }

    auto cbv = SortFeaturesAndBuildCBV(move(features));
    callback(*cbv);

    return true;
  }

private:
  vector<pair<uint32_t, m2::PointD>> m_features;
  size_t m_lastReported;
};

// This class represents a slow retrieval strategy.  It starts with
// initial viewport and iteratively scales it until whole mwm is
// covered by a scaled viewport.  On each scale it retrieves features
// for a scaled viewport from a geometry index and then intersects
// them with features retrieved from search index.
class SlowPathStrategy : public Retrieval::Strategy
{
public:
  SlowPathStrategy(MwmSet::MwmHandle & handle, m2::RectD const & viewport,
                   SearchQueryParams const & params,
                   unique_ptr<coding::CompressedBitVector> && addressFeatures)
    : Strategy(handle, viewport)
    , m_params(params)
    , m_nonReported(move(addressFeatures))
    , m_coverageScale(0)
  {
    ASSERT(m_nonReported.get(), ("Strategy must be initialized with valid address features set."));

    // No need to initialize slow path strategy when there are no
    // features at all.
    if (m_nonReported->PopCount() == 0)
      return;

    auto * value = m_handle.GetValue<MwmValue>();
    ASSERT(value, ());
    feature::DataHeader const & header = value->GetHeader();
    auto const scaleRange = header.GetScaleRange();
    m_coverageScale = min(max(m_params.m_scale, scaleRange.first), scaleRange.second);
    m_bounds = header.GetBounds();
  }

  // Retrieval::Strategy overrides:
  bool RetrieveImpl(double scale, my::Cancellable const & cancellable,
                    TCallback const & callback) override
  {
    m2::RectD currViewport = m_viewport;
    currViewport.Scale(scale);

    // Early exit when scaled viewport does not intersect mwm bounds.
    if (!currViewport.Intersect(m_bounds))
      return true;

    // Early exit when all features from this mwm were already
    // reported.
    if (!m_nonReported || m_nonReported->PopCount() == 0)
      return true;

    unique_ptr<coding::CompressedBitVector> geometryFeatures;

    // Early exit when whole mwm is inside scaled viewport.
    if (currViewport.IsRectInside(m_bounds))
    {
      geometryFeatures.swap(m_nonReported);
      callback(*geometryFeatures);
      return true;
    }

    try
    {
      if (m_prevScale < 0)
      {
        covering::IntervalsT coverage;
        v2::CoverRect(currViewport, m_coverageScale, coverage);

        /// @todo I didn't find a better solution for now to share MwmHandle.
        v2::MwmContext context(move(m_handle));
        geometryFeatures = RetrieveGeometryFeaturesImpl(context, cancellable,
                                                        coverage, m_coverageScale);
        m_handle = move(context.m_handle);

        for (auto const & interval : coverage)
          m_visited.Add(interval);
      }
      else
      {
        m2::RectD prevViewport = m_viewport;
        prevViewport.Scale(m_prevScale);

        m2::RectD a(currViewport.LeftTop(), prevViewport.RightTop());
        m2::RectD c(currViewport.RightBottom(), prevViewport.LeftBottom());
        m2::RectD b(a.RightTop(), c.RightTop());
        m2::RectD d(a.LeftBottom(), c.LeftBottom());

        covering::IntervalsT coverage;
        v2::CoverRect(a, m_coverageScale, coverage);
        v2::CoverRect(b, m_coverageScale, coverage);
        v2::CoverRect(c, m_coverageScale, coverage);
        v2::CoverRect(d, m_coverageScale, coverage);

        my::SortUnique(coverage);
        coverage = covering::SortAndMergeIntervals(coverage);
        coverage.erase(remove_if(coverage.begin(), coverage.end(),
                                 [this](covering::IntervalT const & interval)
                                 {
                                   return m_visited.Elems().count(interval) != 0;
                                 }),
                       coverage.end());

        covering::IntervalsT reducedCoverage;
        for (auto const & interval : coverage)
          m_visited.SubtractFrom(interval, reducedCoverage);

        /// @todo I didn't find a better solution for now to share MwmHandle.
        v2::MwmContext context(move(m_handle));
        geometryFeatures = RetrieveGeometryFeaturesImpl(context, cancellable,
                                                        reducedCoverage, m_coverageScale);
        m_handle = move(context.m_handle);

        for (auto const & interval : reducedCoverage)
          m_visited.Add(interval);
      }
    }
    catch (CancelException &)
    {
      return false;
    }

    auto toReport = coding::CompressedBitVector::Intersect(*m_nonReported, *geometryFeatures);
    m_nonReported = coding::CompressedBitVector::Subtract(*m_nonReported, *toReport);
    callback(*toReport);
    return true;
  }

private:
  SearchQueryParams const & m_params;

  unique_ptr<coding::CompressedBitVector> m_nonReported;

  // This set is used to accumulate all read intervals from mwm and
  // prevent further reads from the same offsets.
  my::IntervalSet<int64_t> m_visited;

  m2::RectD m_bounds;
  int m_coverageScale;
};
}  // namespace

// Retrieval::Limits -------------------------------------------------------------------------------
Retrieval::Limits::Limits()
  : m_maxNumFeatures(0)
  , m_maxViewportScale(0.0)
  , m_maxNumFeaturesSet(false)
  , m_maxViewportScaleSet(false)
  , m_searchInWorld(false)
{
}

void Retrieval::Limits::SetMaxNumFeatures(uint64_t maxNumFeatures)
{
  m_maxNumFeatures = maxNumFeatures;
  m_maxNumFeaturesSet = true;
}

uint64_t Retrieval::Limits::GetMaxNumFeatures() const
{
  ASSERT(IsMaxNumFeaturesSet(), ());
  return m_maxNumFeatures;
}

void Retrieval::Limits::SetMaxViewportScale(double maxViewportScale)
{
  m_maxViewportScale = maxViewportScale;
  m_maxViewportScaleSet = true;
}

double Retrieval::Limits::GetMaxViewportScale() const
{
  ASSERT(IsMaxViewportScaleSet(), ());
  return m_maxViewportScale;
}

// Retrieval::Strategy -----------------------------------------------------------------------------
Retrieval::Strategy::Strategy(MwmSet::MwmHandle & handle, m2::RectD const & viewport)
  : m_handle(handle), m_viewport(viewport), m_prevScale(-numeric_limits<double>::epsilon())
{
}

bool Retrieval::Strategy::Retrieve(double scale, my::Cancellable const & cancellable,
                                   TCallback const & callback)
{
  ASSERT_GREATER(scale, m_prevScale, ("Invariant violation."));
  bool result = RetrieveImpl(scale, cancellable, callback);
  m_prevScale = scale;
  return result;
}

// Retrieval::Bucket -------------------------------------------------------------------------------
Retrieval::Bucket::Bucket(MwmSet::MwmHandle && handle)
  : m_handle(move(handle))
  , m_featuresReported(0)
  , m_numAddressFeatures(0)
  , m_intersectsWithViewport(false)
  , m_finished(false)
{
  auto * value = m_handle.GetValue<MwmValue>();
  ASSERT(value, ());
  feature::DataHeader const & header = value->GetHeader();
  m_bounds = header.GetBounds();
}

// Retrieval ---------------------------------------------------------------------------------------
Retrieval::Retrieval() : m_index(nullptr), m_featuresReported(0) {}

// static
unique_ptr<coding::CompressedBitVector> Retrieval::RetrieveAddressFeatures(
    MwmSet::MwmId const & id, MwmValue & value, my::Cancellable const & cancellable,
    SearchQueryParams const & params)
{
  version::MwmTraits mwmTraits(value.GetMwmVersion().GetFormat());

  if (mwmTraits.GetSearchIndexFormat() ==
      version::MwmTraits::SearchIndexFormat::FeaturesWithRankAndCenter)
  {
    using TValue = FeatureWithRankAndCenter;
    return RetrieveAddressFeaturesImpl<TValue>(id, value, cancellable, params);
  }
  else if (mwmTraits.GetSearchIndexFormat() ==
           version::MwmTraits::SearchIndexFormat::CompressedBitVector)
  {
    using TValue = FeatureIndexValue;
    return RetrieveAddressFeaturesImpl<TValue>(id, value, cancellable, params);
  }
  return unique_ptr<coding::CompressedBitVector>();
}

// static
unique_ptr<coding::CompressedBitVector> Retrieval::RetrieveGeometryFeatures(
    v2::MwmContext const & context, my::Cancellable const & cancellable,
    m2::RectD const & rect, int scale)
{
  covering::IntervalsT coverage;
  v2::CoverRect(rect, scale, coverage);
  return RetrieveGeometryFeaturesImpl(context, cancellable, coverage, scale);
}

void Retrieval::Init(Index & index, vector<shared_ptr<MwmInfo>> const & infos,
                     m2::RectD const & viewport, SearchQueryParams const & params,
                     Limits const & limits)
{
  Release();

  m_index = &index;
  m_viewport = viewport;
  m_params = params;
  m_limits = limits;
  m_featuresReported = 0;

  for (auto const & info : infos)
  {
    MwmSet::MwmHandle handle = index.GetMwmHandleById(MwmSet::MwmId(info));
    if (!handle.IsAlive())
      continue;
    auto * value = handle.GetValue<MwmValue>();
    if (!value || !value->m_cont.IsExist(SEARCH_INDEX_FILE_TAG) ||
        !value->m_cont.IsExist(INDEX_FILE_TAG))
    {
      continue;
    }
    bool const isWorld = value->GetHeader().GetType() == feature::DataHeader::world;
    if (isWorld && !m_limits.GetSearchInWorld())
      continue;
    m_buckets.emplace_back(move(handle));
  }
}

void Retrieval::Go(Callback & callback)
{
  static double const kViewportScaleMul = sqrt(2.0);

  double currScale = 1.0;
  while (true)
  {
    if (IsCancelled())
      break;

    double reducedScale = currScale;
    if (m_limits.IsMaxViewportScaleSet() && reducedScale >= m_limits.GetMaxViewportScale())
      reducedScale = m_limits.GetMaxViewportScale();

    for (auto & bucket : m_buckets)
    {
      if (!RetrieveForScale(bucket, reducedScale, callback))
        break;
    }

    if (Finished())
      break;

    if (m_limits.IsMaxViewportScaleSet() && reducedScale >= m_limits.GetMaxViewportScale())
      break;
    if (m_limits.IsMaxNumFeaturesSet() && m_featuresReported >= m_limits.GetMaxNumFeatures())
      break;

    currScale *= kViewportScaleMul;
  }
}

void Retrieval::Release() { m_buckets.clear(); }

bool Retrieval::RetrieveForScale(Bucket & bucket, double scale, Callback & callback)
{
  if (IsCancelled())
    return false;

  m2::RectD viewport = m_viewport;
  viewport.Scale(scale);
  if (scales::GetScaleLevelD(viewport) <= kMaxViewportScaleLevel)
  {
    viewport.MakeInfinite();
    scale = kInfinity;
  }

  if (bucket.m_finished || !viewport.IsIntersect(bucket.m_bounds))
    return true;

  if (!bucket.m_intersectsWithViewport)
  {
    // This is the first time viewport intersects with
    // mwm. Initialize bucket's retrieval strategy.
    if (!InitBucketStrategy(bucket, scale))
      return false;
    bucket.m_intersectsWithViewport = true;
  }

  ASSERT(bucket.m_intersectsWithViewport, ());
  ASSERT_LESS_OR_EQUAL(bucket.m_featuresReported, bucket.m_numAddressFeatures, ());
  if (bucket.m_featuresReported == bucket.m_numAddressFeatures)
  {
    // All features were reported for the bucket, mark it as
    // finished and move to the next bucket.
    FinishBucket(bucket, callback);
    return true;
  }

  auto wrapper = [&](coding::CompressedBitVector const & features)
  {
    ReportFeatures(bucket, features, scale, callback);
  };

  if (!bucket.m_strategy->Retrieve(scale, *this /* cancellable */, wrapper))
    return false;

  if (viewport.IsRectInside(bucket.m_bounds))
  {
    // Viewport completely covers the bucket, so mark it as finished
    // and switch to the next bucket. Note that "viewport covers the
    // bucket" is not the same as "all features from the bucket were
    // reported", because of scale parameter. Search index reports
    // all matching features, but geometry index can skip features
    // from more detailed scales.
    FinishBucket(bucket, callback);
  }

  return true;
}

bool Retrieval::InitBucketStrategy(Bucket & bucket, double scale)
{
  ASSERT(!bucket.m_strategy, ());
  ASSERT_EQUAL(0, bucket.m_numAddressFeatures, ());

  unique_ptr<coding::CompressedBitVector> addressFeatures;

  try
  {
    addressFeatures =
        RetrieveAddressFeatures(bucket.m_handle.GetId(), *bucket.m_handle.GetValue<MwmValue>(),
                                *this /* cancellable */, m_params);
  }
  catch (CancelException &)
  {
    return false;
  }

  ASSERT(addressFeatures.get(), ("Can't retrieve address features."));
  bucket.m_numAddressFeatures = addressFeatures->PopCount();

  if (bucket.m_numAddressFeatures < kFastPathThreshold)
  {
    bucket.m_strategy.reset(
        new FastPathStrategy(*m_index, bucket.m_handle, m_viewport, move(addressFeatures)));
  }
  else
  {
    bucket.m_strategy.reset(
        new SlowPathStrategy(bucket.m_handle, m_viewport, m_params, move(addressFeatures)));
  }

  return true;
}

void Retrieval::FinishBucket(Bucket & bucket, Callback & callback)
{
  if (bucket.m_finished)
    return;

  auto const mwmId = bucket.m_handle.GetId();

  bucket.m_finished = true;
  bucket.m_handle = MwmSet::MwmHandle();

  callback.OnMwmProcessed(mwmId);
}

bool Retrieval::Finished() const
{
  for (auto const & bucket : m_buckets)
  {
    if (!bucket.m_finished)
      return false;
  }
  return true;
}

void Retrieval::ReportFeatures(Bucket & bucket, coding::CompressedBitVector const & features,
                               double scale, Callback & callback)
{
  if (m_limits.IsMaxNumFeaturesSet() && m_featuresReported >= m_limits.GetMaxNumFeatures())
    return;

  if (features.PopCount() == 0)
    return;

  callback.OnFeaturesRetrieved(bucket.m_handle.GetId(), scale, features);
  bucket.m_featuresReported += features.PopCount();
  m_featuresReported += features.PopCount();
}
}  // namespace search
