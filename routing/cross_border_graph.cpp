#include "routing/cross_border_graph.hpp"

#include "geometry/mercator.hpp"

namespace routing
{
void CrossBorderGraph::AddCrossBorderSegment(RegionSegmentId segId, CrossBorderSegment const & segment)
{
  m_segments.emplace(segId, segment);

  auto addEndingToMwms = [&](CrossBorderSegmentEnding const & ending)
  {
    auto [it, inserted] = m_mwms.emplace(ending.m_numMwmId, std::vector<RegionSegmentId>{segId});
    if (!inserted)
      it->second.push_back(segId);
  };

  addEndingToMwms(segment.m_start);
  addEndingToMwms(segment.m_end);
}

CrossBorderSegmentEnding::CrossBorderSegmentEnding(m2::PointD const & point, NumMwmId const & mwmId)
  : m_point(mercator::ToLatLon(point), geometry::kDefaultAltitudeMeters)
  , m_numMwmId(mwmId)
{}

CrossBorderSegmentEnding::CrossBorderSegmentEnding(ms::LatLon const & point, NumMwmId const & mwmId)
  : m_point(point, 0)
  , m_numMwmId(mwmId)
{}

CrossBorderGraphSerializer::Header::Header(CrossBorderGraph const & graph, uint32_t version)
  : m_numRegions(static_cast<uint32_t>(graph.m_mwms.size()))
  , m_numRoads(static_cast<uint32_t>(graph.m_segments.size()))
  , m_version(version)
{}

// static
uint32_t CrossBorderGraphSerializer::Hash(std::string const & s)
{
  // We use RSHash (Real Simple Hash) variation. RSHash is a standard hash calculating function
  // described in Robert Sedgwick's "Algorithms in C". The difference between this implementation
  // and the original algorithm is that we return hash, not (hash & & 0x7FFFFFFF).
  uint32_t constexpr b = 378551;
  uint32_t a = 63689;

  uint32_t hash = 0;

  for (auto c : s)
  {
    hash = hash * a + c;
    a *= b;
  }

  return hash;
}
}  // namespace routing
