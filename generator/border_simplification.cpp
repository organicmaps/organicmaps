#include "generator/border_simplification.hpp"

#include "geometry/simplification.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <utility>

#include <boost/geometry/geometries/register/point.hpp>
#include "std/boost_geometry.hpp"

BOOST_GEOMETRY_REGISTER_POINT_2D(m2::PointD, double, boost::geometry::cs::cartesian, x, y)

namespace borders
{
namespace
{
using VertexId = uint32_t;
using Arc = std::vector<VertexId>;

bool IsValidRing(std::vector<m2::PointD> const & points)
{
  if (points.size() < 3)
    return false;
  boost::geometry::model::ring<m2::PointD> ring(points.begin(), points.end());
  // Winding and an explicit closing record do not affect the generator's fill.
  boost::geometry::correct(ring);
  return boost::geometry::is_valid(ring);
}

struct Vertex
{
  VertexId m_first = 0;
  VertexId m_last = 0;
  bool m_seen = false;
  bool m_fixed = false;
};

struct Ring
{
  Arc m_ids;
  std::vector<size_t> m_junctions;
  bool m_valid = false;
};

Arc CanonicalArc(Arc ids, bool cyclic)
{
  if (cyclic)
  {
    ASSERT_EQUAL(ids.front(), ids.back(), ());
    ids.pop_back();
    std::rotate(ids.begin(), std::min_element(ids.begin(), ids.end()), ids.end());
    ids.push_back(ids.front());
  }
  Arc reversed(ids.rbegin(), ids.rend());
  if (reversed < ids)
    return reversed;
  return ids;
}

template <typename Fn>
void ForEachArc(Ring const & ring, Fn && fn)
{
  auto const & ids = ring.m_ids;
  if (ring.m_junctions.empty())
  {
    Arc arc = ids;
    arc.push_back(arc.front());
    fn(arc, true);
    return;
  }
  for (size_t i = 0; i < ring.m_junctions.size(); ++i)
  {
    size_t const first = ring.m_junctions[i];
    size_t const last =
        i + 1 < ring.m_junctions.size() ? ring.m_junctions[i + 1] : ring.m_junctions.front() + ids.size();
    Arc arc;
    arc.reserve(last - first + 1);
    for (size_t position = first; position <= last; ++position)
      arc.push_back(ids[position % ids.size()]);
    fn(arc, false);
  }
}

struct SimplifiedArc
{
  Arc m_ids;
  bool m_locked = false;
};
}  // namespace

std::vector<m2::RegionD> SimplifyBorders(std::vector<m2::RegionD> const & borders, double squareEpsilon)
{
  CHECK_GREATER(squareEpsilon, 0.0, ());
  if (borders.empty())
    return {};

  LOG(LINFO, ("Building shared border topology for", borders.size(), "rings"));
  size_t recordCount = 0;
  for (auto const & border : borders)
    recordCount += border.Size();
  std::vector<m2::PointD> coordinates;
  coordinates.reserve(recordCount);
  for (auto const & border : borders)
    coordinates.insert(coordinates.end(), border.Begin(), border.End());
  std::sort(coordinates.begin(), coordinates.end());
  coordinates.erase(std::unique(coordinates.begin(), coordinates.end()), coordinates.end());
  CHECK_LESS(coordinates.size(), std::numeric_limits<VertexId>::max(), ());

  auto vertexId = [&](m2::PointD const & point)
  {
    auto const it = std::lower_bound(coordinates.begin(), coordinates.end(), point);
    ASSERT(it != coordinates.end() && *it == point, (point));
    return static_cast<VertexId>(it - coordinates.begin());
  };
  auto pointsFor = [&](Arc const & ids)
  {
    std::vector<m2::PointD> points;
    points.reserve(ids.size());
    for (auto id : ids)
      points.push_back(coordinates[id]);
    return points;
  };

  std::vector<Vertex> vertices(coordinates.size());
  std::vector<Ring> rings;
  rings.reserve(borders.size());
  for (auto const & border : borders)
  {
    auto & ring = rings.emplace_back();
    ring.m_ids.reserve(border.Size());
    for (auto const & point : border.Data())
    {
      auto const id = vertexId(point);
      if (ring.m_ids.empty() || id != ring.m_ids.back())
        ring.m_ids.push_back(id);
    }
    if (ring.m_ids.size() > 1 && ring.m_ids.front() == ring.m_ids.back())
      ring.m_ids.pop_back();
    CHECK(!ring.m_ids.empty(), ());
    ring.m_valid = IsValidRing(pointsFor(ring.m_ids));
    auto occurrences = ring.m_ids;
    std::sort(occurrences.begin(), occurrences.end());
    for (size_t i = 1; i < occurrences.size(); ++i)
      if (occurrences[i] == occurrences[i - 1])
        vertices[occurrences[i]].m_fixed = true;
    for (size_t i = 0; i < ring.m_ids.size(); ++i)
    {
      auto const previous = ring.m_ids[(i + ring.m_ids.size() - 1) % ring.m_ids.size()];
      auto const following = ring.m_ids[(i + 1) % ring.m_ids.size()];
      auto & vertex = vertices[ring.m_ids[i]];
      auto const [first, last] = std::minmax(previous, following);
      if (vertex.m_seen && (vertex.m_first != first || vertex.m_last != last))
        vertex.m_fixed = true;
      vertex.m_first = first;
      vertex.m_last = last;
      vertex.m_seen = true;
    }
  }
  for (auto & ring : rings)
    for (size_t i = 0; i < ring.m_ids.size(); ++i)
      if (vertices[ring.m_ids[i]].m_fixed)
        ring.m_junctions.push_back(i);
  std::vector<Vertex>().swap(vertices);

  std::map<Arc, SimplifiedArc> cache;
  auto retainArc = [&](Arc const & arc, bool cyclic)
  {
    if (arc.size() <= 2)
      return arc;
    auto key = CanonicalArc(arc, cyclic);
    auto const [it, inserted] = cache.try_emplace(key);
    auto & entry = it->second;
    if (inserted)
    {
      auto const points = pointsFor(key);
      SimplifyDP(points.begin(), points.end(), squareEpsilon, m2::SquaredDistanceFromSegmentToPoint(),
                 [&](m2::PointD const & point) { entry.m_ids.push_back(vertexId(point)); });
    }
    Arc retained = entry.m_locked ? key : entry.m_ids;
    if (!cyclic && arc != key)
      std::reverse(retained.begin(), retained.end());
    return retained;
  };

  for (size_t pass = 1;; ++pass)
  {
    LOG(LINFO, ("Simplifying and validating shared borders, pass", pass));
    std::vector<m2::RegionD> result;
    result.reserve(borders.size());
    std::vector<size_t> failed;
    for (size_t i = 0; i < rings.size(); ++i)
    {
      auto const & ring = rings[i];
      Arc selected;
      ForEachArc(ring, [&](Arc const & arc, bool cyclic)
      {
        auto const retained = retainArc(arc, cyclic);
        selected.insert(selected.end(), retained.begin() + (selected.empty() ? 0 : 1), retained.end());
      });
      if (selected.size() > 1 && selected.front() == selected.back())
        selected.pop_back();
      // Preserve source order and closing records when no vertices were removed.
      if (selected.size() == ring.m_ids.size())
      {
        result.push_back(borders[i]);
        continue;
      }
      auto points = pointsFor(selected);
      if (points.size() < 3 || (ring.m_valid && !IsValidRing(points)))
        failed.push_back(i);
      result.emplace_back(std::move(points));
    }
    if (failed.empty())
      return result;

    size_t locked = 0;
    for (auto i : failed)
      ForEachArc(rings[i], [&](Arc const & arc, bool cyclic)
      {
        if (arc.size() <= 2)
          return;
        auto const it = cache.find(CanonicalArc(arc, cyclic));
        CHECK(it != cache.end(), (i));
        if (!it->second.m_locked)
        {
          it->second.m_locked = true;
          ++locked;
        }
      });
    CHECK_GREATER(locked, 0, ("Invalid border despite retaining all source arcs", failed));
  }
}
}  // namespace borders
