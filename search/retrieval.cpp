#include "search/retrieval.hpp"

#include "search/feature_offset_match.hpp"

#include "indexer/index.hpp"
#include "indexer/search_trie.hpp"

#include "coding/reader_wrapper.hpp"

#include "base/stl_add.hpp"

#include "std/algorithm.hpp"
#include "std/cmath.hpp"

namespace search
{
namespace
{
struct EmptyFilter
{
  inline bool operator()(uint32_t /* featureId */) const { return true; }
};

void RetrieveAddressFeatures(MwmSet::MwmHandle const & handle, SearchQueryParams const & params,
                             vector<uint32_t> & featureIds)
{
  auto * value = handle.GetValue<MwmValue>();
  ASSERT(value, ());
  serial::CodingParams codingParams(trie::GetCodingParams(value->GetHeader().GetDefCodingParams()));
  ModelReaderPtr searchReader = value->m_cont.GetReader(SEARCH_INDEX_FILE_TAG);
  unique_ptr<trie::DefaultIterator> const trieRoot(
      trie::ReadTrie(SubReaderWrapper<Reader>(searchReader.GetPtr()),
                     trie::ValueReader(codingParams), trie::TEdgeValueReader()));

  featureIds.clear();
  auto collector = [&](trie::ValueReader::ValueType const & value)
  {
    featureIds.push_back(value.m_featureId);
  };
  MatchFeaturesInTrie(params, *trieRoot, EmptyFilter(), collector);
}

void RetrieveGeometryFeatures(MwmSet::MwmHandle const & handle, m2::RectD const & viewport,
                              SearchQueryParams const & params, vector<uint32_t> & featureIds)
{
  auto * value = handle.GetValue<MwmValue>();
  ASSERT(value, ());
  feature::DataHeader const & header = value->GetHeader();
  ASSERT(viewport.IsIntersect(header.GetBounds()), ());

  auto const scaleRange = header.GetScaleRange();
  int const scale = min(max(params.m_scale, scaleRange.first), scaleRange.second);

  covering::CoveringGetter covering(viewport, covering::ViewportWithLowLevels);
  covering::IntervalsT const & intervals = covering.Get(scale);
  ScaleIndex<ModelReaderPtr> index(value->m_cont.GetReader(INDEX_FILE_TAG), value->m_factory);

  featureIds.clear();
  auto collector = MakeBackInsertFunctor(featureIds);
  for (auto const & interval : intervals)
  {
    index.ForEachInIntervalAndScale(collector, interval.first, interval.second, scale);
  }
}
}  // namespace

Retrieval::Limits::Limits()
    : m_maxNumFeatures(0),
      m_maxViewportScale(0.0),
      m_maxNumFeaturesSet(false),
      m_maxViewportScaleSet(false)
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

Retrieval::Bucket::Bucket(MwmSet::MwmHandle && handle)
    : m_handle(move(handle)),
      m_intersectsWithViewport(false),
      m_coveredByViewport(false),
      m_finished(false)
{
  auto * value = m_handle.GetValue<MwmValue>();
  ASSERT(value, ());
  feature::DataHeader const & header = value->GetHeader();
  m_bounds = header.GetBounds();
}

Retrieval::Retrieval() : m_index(nullptr), m_featuresReported(0) {}

void Retrieval::Init(Index & index, m2::RectD const & viewport, SearchQueryParams const & params,
                     Limits const & limits)
{
  m_index = &index;
  m_viewport = viewport;
  m_params = params;
  m_limits = limits;
  m_featuresReported = 0;

  vector<shared_ptr<MwmInfo>> infos;
  index.GetMwmsInfo(infos);

  m_buckets.clear();
  for (auto const & info : infos)
  {
    MwmSet::MwmHandle handle = index.GetMwmHandleById(MwmSet::MwmId(info));
    if (!handle.IsAlive())
      continue;
    auto * value = handle.GetValue<MwmValue>();
    if (value && value->m_cont.IsExist(SEARCH_INDEX_FILE_TAG) &&
        value->m_cont.IsExist(INDEX_FILE_TAG))
    {
      m_buckets.emplace_back(move(handle));
    }
  }
}

void Retrieval::Go(Callback & callback)
{
  static double const kViewportScaleMul = sqrt(2.0);

  for (double viewportScale = 1.0;; viewportScale *= kViewportScaleMul)
  {
    double scale = viewportScale;
    if (m_limits.IsMaxViewportScaleSet() && scale >= m_limits.GetMaxViewportScale())
      scale = m_limits.GetMaxViewportScale();

    m2::RectD viewport = m_viewport;
    viewport.Scale(scale);
    RetrieveForViewport(viewport, callback);

    if (ViewportCoversAllMwms())
      break;
    if (m_limits.IsMaxViewportScaleSet() && scale >= m_limits.GetMaxViewportScale())
      break;
    if (m_limits.IsMaxNumFeaturesSet() && m_featuresReported >= m_limits.GetMaxNumFeatures())
      break;
  }

  for (auto & bucket : m_buckets)
  {
    if (bucket.m_finished)
      continue;
    // The bucket is not covered by viewport, thus all matching
    // features were not reported.
    bucket.m_finished = true;
    ReportFeatures(bucket, callback);
  }
}

void Retrieval::RetrieveForViewport(m2::RectD const & viewport, Callback & callback)
{
  for (auto & bucket : m_buckets)
  {
    if (bucket.m_coveredByViewport || bucket.m_finished || !viewport.IsIntersect(bucket.m_bounds))
      continue;

    if (!bucket.m_intersectsWithViewport)
    {
      // This is the first time viewport intersects with mwm. Retrieve
      // all matching features from search index.
      RetrieveAddressFeatures(bucket.m_handle, m_params, bucket.m_addressFeatures);
      sort(bucket.m_addressFeatures.begin(), bucket.m_addressFeatures.end());
      bucket.m_intersectsWithViewport = true;
    }

    // Mwm is still not covered by expanding viewport.
    if (!bucket.m_coveredByViewport)
    {
      RetrieveGeometryFeatures(bucket.m_handle, viewport, m_params, bucket.m_geometryFeatures);
      sort(bucket.m_geometryFeatures.begin(), bucket.m_geometryFeatures.end());

      bucket.m_intersection.clear();
      set_intersection(bucket.m_addressFeatures.begin(), bucket.m_addressFeatures.end(),
                       bucket.m_geometryFeatures.begin(), bucket.m_geometryFeatures.end(),
                       back_inserter(bucket.m_intersection));
    }

    if (!bucket.m_coveredByViewport && viewport.IsRectInside(bucket.m_bounds))
    {
      // Next time we will skip the bucket, so it's better to report
      // all its features now.
      bucket.m_coveredByViewport = true;
      bucket.m_finished = true;
      ReportFeatures(bucket, callback);
    }
  }
}

bool Retrieval::ViewportCoversAllMwms() const
{
  for (auto const & bucket : m_buckets)
  {
    if (!bucket.m_coveredByViewport)
      return false;
  }
  return true;
}

void Retrieval::ReportFeatures(Bucket & bucket, Callback & callback)
{
  ASSERT(!m_limits.IsMaxNumFeaturesSet() || m_featuresReported <= m_limits.GetMaxNumFeatures(), ());
  if (m_limits.IsMaxNumFeaturesSet())
  {
    uint64_t const delta = m_limits.GetMaxNumFeatures() - m_featuresReported;
    if (bucket.m_intersection.size() > delta)
      bucket.m_intersection.resize(delta);
  }
  if (!bucket.m_intersection.empty())
  {
    callback.OnMwmProcessed(bucket.m_handle.GetId(), bucket.m_intersection);
    m_featuresReported += bucket.m_intersection.size();
  }
}
}  // namespace search
