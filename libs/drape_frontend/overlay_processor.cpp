#include "drape_frontend/overlay_processor.hpp"

#include "coding/point_coding.hpp"

#include "geometry/clipping.hpp"

#include "base/logging.hpp"

#include <algorithm>

namespace df
{
namespace
{
struct EndpointRef
{
  size_t featureIdx;
  bool isStart;
};

using EndpointMap = std::unordered_multimap<int64_t, EndpointRef>;

int64_t PointKey(m2::PointD const & pt)
{
  return PointToInt64Obsolete(pt, kPointCoordBits);
}

/// Direction at the end (atEnd=true) or start (atEnd=false) of a polyline.
m2::PointD GetDirection(std::vector<m2::PointD> const & pts, bool atEnd)
{
  ASSERT_GREATER(pts.size(), 1, ());
  if (atEnd)
    return (pts.back() - pts[pts.size() - 2]).Normalize();
  return (pts[1] - pts[0]).Normalize();
}

/// Direction a candidate would continue in if merged at its start or end.
m2::PointD GetCandidateDirection(std::vector<m2::PointD> const & pts, bool isStart)
{
  ASSERT_GREATER(pts.size(), 1, ());
  if (isStart)
    return (pts[1] - pts[0]).Normalize();
  return (pts[pts.size() - 2] - pts.back()).Normalize();
}

/// Greedy merge + orient + clip for a generic FeatureData type.
/// FeatureDataT must have m_points and m_params fields.
template <class FeatureDataT, class FnT>
void MergeImpl(std::vector<FeatureDataT> const & features, FnT && fn)
{
  size_t const n = features.size();
  std::vector<bool> used(n, false);

  EndpointMap endpointMap;
  endpointMap.reserve(n * 2);
  for (size_t i = 0; i < n; ++i)
  {
    auto const & pts = features[i].m_spline->GetPath();
    endpointMap.insert({PointKey(pts.front()), {i, true}});
    endpointMap.insert({PointKey(pts.back()), {i, false}});
  }

  std::vector<size_t> chainIndices;  // Indices of features merged into current chain.

  auto const extend = [&](m2::PointD const & pt, std::vector<m2::PointD> & points, bool forward
#ifdef DEBUG_OVERLAY_PROCESSOR
                          ,
                          std::vector<m2::PointD> & junctions
#endif
                          ) -> bool
  {
    auto const range = endpointMap.equal_range(PointKey(pt));

    // Find the best candidate that continues in the same direction.
    // For forward: chain direction at tail, candidate direction leaving junction.
    // For backward: chain direction at front, candidate direction arriving at junction.
    m2::PointD const chainDir = GetDirection(points, forward);

    double bestDot = -2.0;
    int bestIdx = -1;
    bool bestIsStart = false;

    for (auto it = range.first; it != range.second; ++it)
    {
      size_t const idx = it->second.featureIdx;
      if (used[idx])
        continue;

      auto const & pts = features[idx].m_spline->GetPath();

      // Direction the candidate contributes at the junction after merge.
      m2::PointD candidateDir = it->second.isStart
                                  ? GetCandidateDirection(pts, true)    // continues from start
                                  : GetCandidateDirection(pts, false);  // reversed: end→start direction at junction
      if (!forward)
        candidateDir *= -1.0;

      double const dot = m2::DotProduct(chainDir, candidateDir);
      if (dot > bestDot)
      {
        bestDot = dot;
        bestIdx = idx;
        bestIsStart = it->second.isStart;
      }
    }

    if (bestIdx == -1)
      return false;

    auto const & pts = features[bestIdx].m_spline->GetPath();
#ifdef DEBUG_OVERLAY_PROCESSOR
    junctions.push_back(pt);
#endif
    // clang-format off
    if (forward)
    {
      if (bestIsStart)
        points.insert(points.end(), pts.begin() + 1, pts.end());
      else
        points.insert(points.end(), pts.rbegin() + 1, pts.rend());
    }
    else
    {
      if (bestIsStart)
        points.insert(points.begin(), pts.rbegin(), pts.rend() - 1);
      else
        points.insert(points.begin(), pts.begin(), pts.end() - 1);
    }
    // clang-format on

    used[bestIdx] = true;
    chainIndices.push_back(bestIdx);
    return true;
  };

  for (size_t seed = 0; seed < n; ++seed)
  {
    if (used[seed])
      continue;
    used[seed] = true;
    chainIndices.clear();
    chainIndices.push_back(seed);

    auto points = features[seed].m_spline->GetPath();

    // clang-format off
#ifdef DEBUG_OVERLAY_PROCESSOR
    std::vector<m2::PointD> junctions;
    while (extend(points.back(), points, true, junctions)) {}
    while (extend(points.front(), points, false, junctions)) {}
#else
    while (extend(points.back(), points, true)) {}
    while (extend(points.front(), points, false)) {}
#endif
    // clang-format on

    // Orient left-to-right (or top-to-bottom for vertical roads).
    auto const & front = points.front();
    auto const & back = points.back();
    if (back.x < front.x || (back.x == front.x && back.y < front.y))
      std::reverse(points.begin(), points.end());

    fn(chainIndices, std::move(points)
#ifdef DEBUG_OVERLAY_PROCESSOR
                         ,
       std::move(junctions)
#endif
    );
  }
}

template <class FeatureDataT, class MakeChainFn>
void MergeAndClip(std::unordered_map<std::string, std::vector<FeatureDataT>> & featuresByName,
                  m2::RectD const & tileRect, MakeChainFn && makeChain,
                  std::vector<OverlayProcessor::MergedChain> & result)
{
  for (auto const & [_, features] : featuresByName)
  {
    MergeImpl(features, [&](std::vector<size_t> const & chainIndices, std::vector<m2::PointD> && points
#ifdef DEBUG_OVERLAY_PROCESSOR
                            ,
                            std::vector<m2::PointD> && junctions
#endif
                        )
    {
      OverlayProcessor::MergedChain mc;
      mc.m_clippedSplines = m2::ClipSplineByRect(tileRect, m2::SharedSpline(std::move(points)));
      if (mc.m_clippedSplines.empty())
        return;

      makeChain(features, chainIndices, mc);
#ifdef DEBUG_OVERLAY_PROCESSOR
      mc.m_junctionPoints = std::move(junctions);
#endif

      result.push_back(std::move(mc));
    });
  }

  featuresByName.clear();
}
}  // namespace

OverlayProcessor::OverlayProcessor(m2::RectD const & tileRect, TileKey const & tileKey, double scaleGtoP)
  : m_tileRect(tileRect)
  , m_tileKey(tileKey)
  , m_scaleGtoP(scaleGtoP)
{}

void OverlayProcessor::CollectFeature(std::string const & key, m2::SharedSpline const & spline,
                                      PathTextViewParams const & params, ShieldInfo const & shield, bool hasPT)
{
  ASSERT(!params.m_mainText.empty() || !shield.m_roadShields.empty(), ());
  if (spline->GetSize() > 1)
    m_featuresByName[key].push_back({spline, params, shield, hasPT});
}

void OverlayProcessor::CollectPTFeature(m2::SharedSpline const & spline, PathTextViewParams const & params)
{
  ASSERT(!params.m_mainText.empty(), ());
  if (spline->GetSize() > 1)
    m_ptFeaturesByText[params.m_mainText].push_back({spline, params});
}

std::vector<OverlayProcessor::MergedChain> OverlayProcessor::BuildChains()
{
  std::vector<MergedChain> result;
  MergeAndClip(
      m_featuresByName, m_tileRect,
      [](std::vector<StreetFeatureData> const & features, std::vector<size_t> const & indices, MergedChain & mc)
  {
    mc.m_params = features[indices[0]].m_params;
    mc.m_shield = features[indices[0]].m_shield;
    mc.m_hasPT = base::IsExistIf(indices, [&features](size_t idx) { return features[idx].m_hasPT; });
  }, result);
  return result;
}

std::vector<OverlayProcessor::MergedChain> OverlayProcessor::BuildPTChains()
{
  std::vector<MergedChain> result;
  MergeAndClip(m_ptFeaturesByText, m_tileRect,
               [](std::vector<FeatureData> const & features, std::vector<size_t> const & indices, MergedChain & mc)
  { mc.m_params = features[indices[0]].m_params; }, result);
  return result;
}

std::vector<std::vector<m2::PointD>> OverlayProcessor::MergeSplines(std::vector<m2::SharedSpline> const & splines)
{
  std::vector<FeatureData> features;
  for (auto const & spl : splines)
    features.push_back({spl, {}});

  std::vector<std::vector<m2::PointD>> result;
  MergeImpl(features, [&result](std::vector<size_t> const &, std::vector<m2::PointD> && points
#ifdef DEBUG_OVERLAY_PROCESSOR
                                ,
                                std::vector<m2::PointD> &&
#endif
                      ) { result.push_back(std::move(points)); });
  return result;
}
}  // namespace df
