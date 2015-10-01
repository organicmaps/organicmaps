#include "search/retrieval.hpp"

#include "search/feature_offset_match.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/index.hpp"
#include "indexer/search_trie.hpp"

#include "coding/reader_wrapper.hpp"

#include "base/assert.hpp"
#include "base/interval_set.hpp"

#include "std/algorithm.hpp"
#include "std/cmath.hpp"
#include "std/exception.hpp"
#include "std/limits.hpp"

namespace search
{
namespace
{
// This exception can be thrown by callbacks from deeps of search and
// geometry retrieval for fast cancellation of time-consuming tasks.
//
// TODO (@gorshenin): after merge to master, move this class to
// base/cancellable.hpp.
struct CancelException : public exception
{
};

// Upper bound on a number of features when fast path is used.
// Otherwise, slow path is used.
uint64_t constexpr kFastPathThreshold = 100;

void CoverRect(m2::RectD const & rect, int scale, covering::IntervalsT & result)
{
  covering::CoveringGetter covering(rect, covering::ViewportWithLowLevels);
  auto const & intervals = covering.Get(scale);
  result.insert(result.end(), intervals.begin(), intervals.end());
}

// Retrieves from the search index corresponding to |handle| all
// features matching to |params|.
template <typename ToDo>
void RetrieveAddressFeatures(MwmSet::MwmHandle const & handle, SearchQueryParams const & params,
                             ToDo && toDo)
{
  auto emptyFilter = [](uint32_t /* featureId */) { return true; };

  auto * value = handle.GetValue<MwmValue>();
  ASSERT(value, ());
  serial::CodingParams codingParams(trie::GetCodingParams(value->GetHeader().GetDefCodingParams()));
  ModelReaderPtr searchReader = value->m_cont.GetReader(SEARCH_INDEX_FILE_TAG);
  auto const trieRoot = trie::ReadTrie(SubReaderWrapper<Reader>(searchReader.GetPtr()),
                                       trie::ValueReader(codingParams));

  MatchFeaturesInTrie(params, *trieRoot, emptyFilter, forward<ToDo>(toDo));
}

// Retrieves from the geomery index corresponding to handle all
// features in (and, possibly, around) viewport and executes |toDo| on
// them.
template <typename ToDo>
void RetrieveGeometryFeatures(MwmSet::MwmHandle const & handle,
                              covering::IntervalsT const & covering, int scale, ToDo && toDo)
{
  auto * value = handle.GetValue<MwmValue>();
  ASSERT(value, ());

  ScaleIndex<ModelReaderPtr> index(value->m_cont.GetReader(INDEX_FILE_TAG), value->m_factory);
  for (auto const & interval : covering)
    index.ForEachInIntervalAndScale(toDo, interval.first, interval.second, scale);
}

// This class represents a fast retrieval strategy.  When number of
// matching features in an mwm is small, it is worth computing their
// centers explicitly, by loading geometry from mwm.
class FastPathStrategy : public Retrieval::Strategy
{
public:
  FastPathStrategy(Index const & index, MwmSet::MwmHandle & handle, m2::RectD const & viewport,
                   vector<uint32_t> const & addressFeatures)
    : Strategy(handle, viewport), m_lastReported(0)
  {
    m2::PointD const center = m_viewport.Center();

    Index::FeaturesLoaderGuard loader(index, m_handle.GetId());
    for (auto const & featureId : addressFeatures)
    {
      FeatureType feature;
      loader.GetFeatureByIndex(featureId, feature);
      m_features.emplace_back(featureId, feature::GetCenter(feature, FeatureType::WORST_GEOMETRY));
    }
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

    vector<uint32_t> features;

    ASSERT_LESS_OR_EQUAL(m_lastReported, m_features.size(), ());
    while (m_lastReported < m_features.size() &&
           viewport.IsPointInside(m_features[m_lastReported].second))
    {
      features.push_back(m_features[m_lastReported].first);
      ++m_lastReported;
    }

    callback(features);

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
                   SearchQueryParams const & params, vector<uint32_t> const & addressFeatures)
    : Strategy(handle, viewport), m_params(params), m_coverageScale(0)
  {
    if (addressFeatures.empty())
      return;

    m_nonReported.insert(addressFeatures.begin(), addressFeatures.end());

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
    if (m_nonReported.empty())
      return true;

    vector<uint32_t> geometryFeatures;

    // Early exit when whole mwm is inside scaled viewport.
    if (currViewport.IsRectInside(m_bounds))
    {
      geometryFeatures.assign(m_nonReported.begin(), m_nonReported.end());
      m_nonReported.clear();
      callback(geometryFeatures);
      return true;
    }

    try
    {
      auto collector = [&](uint32_t feature)
      {
        if (cancellable.IsCancelled())
          throw CancelException();

        if (m_nonReported.count(feature) != 0)
        {
          geometryFeatures.push_back(feature);
          m_nonReported.erase(feature);
        }
      };

      if (m_prevScale < 0)
      {
        covering::IntervalsT coverage;
        CoverRect(currViewport, m_coverageScale, coverage);
        RetrieveGeometryFeatures(m_handle, coverage, m_coverageScale, collector);
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
        CoverRect(a, m_coverageScale, coverage);
        CoverRect(b, m_coverageScale, coverage);
        CoverRect(c, m_coverageScale, coverage);
        CoverRect(d, m_coverageScale, coverage);

        sort(coverage.begin(), coverage.end());
        coverage.erase(unique(coverage.begin(), coverage.end()), coverage.end());
        coverage = covering::SortAndMergeIntervals(coverage);
        coverage.erase(
            remove_if(coverage.begin(), coverage.end(), [this](covering::IntervalT const & interval)
                      {
              return m_visited.Elems().count(interval) != 0;
            }),
            coverage.end());

        covering::IntervalsT reducedCoverage;
        for (auto const & interval : coverage)
          m_visited.SubtractFrom(interval, reducedCoverage);

        RetrieveGeometryFeatures(m_handle, reducedCoverage, m_coverageScale, collector);

        for (auto const & interval : reducedCoverage)
          m_visited.Add(interval);
      }
    }
    catch (CancelException &)
    {
      return false;
    }

    callback(geometryFeatures);
    return true;
  }

private:
  SearchQueryParams const & m_params;

  set<uint32_t> m_nonReported;

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

void Retrieval::Init(Index & index, vector<shared_ptr<MwmInfo>> const & infos,
                     m2::RectD const & viewport, SearchQueryParams const & params,
                     Limits const & limits)
{
  m_index = &index;
  m_viewport = viewport;
  m_params = params;
  m_limits = limits;
  m_featuresReported = 0;

  m_buckets.clear();
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

bool Retrieval::RetrieveForScale(Bucket & bucket, double scale, Callback & callback)
{
  m2::RectD viewport = m_viewport;
  viewport.Scale(scale);

  if (IsCancelled())
    return false;

  if (bucket.m_finished || !viewport.IsIntersect(bucket.m_bounds))
    return true;

  if (!bucket.m_intersectsWithViewport)
  {
    // This is the first time viewport intersects with
    // mwm. Initialize bucket's retrieval strategy.
    if (!InitBucketStrategy(bucket))
      return false;
    bucket.m_intersectsWithViewport = true;
    if (bucket.m_addressFeatures.empty())
      bucket.m_finished = true;
  }

  ASSERT(bucket.m_intersectsWithViewport, ());
  ASSERT_LESS_OR_EQUAL(bucket.m_featuresReported, bucket.m_addressFeatures.size(), ());
  if (bucket.m_featuresReported == bucket.m_addressFeatures.size())
  {
    // All features were reported for the bucket, mark it as
    // finished and move to the next bucket.
    FinishBucket(bucket, callback);
    return true;
  }

  auto wrapper = [&](vector<uint32_t> & features)
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

bool Retrieval::InitBucketStrategy(Bucket & bucket)
{
  ASSERT(!bucket.m_strategy, ());
  ASSERT(bucket.m_addressFeatures.empty(), ());

  try
  {
    auto collector = [&](trie::ValueReader::ValueType const & value)
    {
      if (IsCancelled())
        throw CancelException();
      bucket.m_addressFeatures.push_back(value.m_featureId);
    };
    RetrieveAddressFeatures(bucket.m_handle, m_params, collector);
  }
  catch (CancelException &)
  {
    return false;
  }

  if (bucket.m_addressFeatures.size() < kFastPathThreshold)
  {
    bucket.m_strategy.reset(
        new FastPathStrategy(*m_index, bucket.m_handle, m_viewport, bucket.m_addressFeatures));
  }
  else
  {
    bucket.m_strategy.reset(
        new SlowPathStrategy(bucket.m_handle, m_viewport, m_params, bucket.m_addressFeatures));
  }

  return true;
}

void Retrieval::FinishBucket(Bucket & bucket, Callback & callback)
{
  if (bucket.m_finished)
    return;
  bucket.m_finished = true;
  bucket.m_handle = MwmSet::MwmHandle();
  callback.OnMwmProcessed(bucket.m_handle.GetId());
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

void Retrieval::ReportFeatures(Bucket & bucket, vector<uint32_t> & featureIds, double scale,
                               Callback & callback)
{
  if (featureIds.empty())
    return;
  callback.OnFeaturesRetrieved(bucket.m_handle.GetId(), scale, featureIds);
  bucket.m_featuresReported += featureIds.size();
  m_featuresReported += featureIds.size();
}
}  // namespace search
