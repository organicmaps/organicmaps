#include "indexer/terrain/isolines_tracer.hpp"

#include "indexer/terrain/terrain_data.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <span>

namespace terrain
{
namespace
{
uint32_t constexpr kNone = std::numeric_limits<uint32_t>::max();

int32_t DisplayAltitude(int32_t altitude, measurement_utils::Units units)
{
  return units == measurement_utils::Units::Imperial
           ? static_cast<int32_t>(std::lround(measurement_utils::MetersToFeet(altitude)))
           : altitude;
}

struct EdgeLevel
{
  uint64_t m_a = 0, m_b = 0;
  int32_t m_level = 0;
  bool operator==(EdgeLevel const &) const = default;
};

struct Fragment
{
  Isoline m_line;
  EdgeLevel m_entry, m_exit;
};

struct TinView
{
  TileMesh const & m_mesh;
  std::vector<int32_t> m_feet;
  std::span<int32_t const> m_altitudes;
  ska::flat_hash_map<uint64_t, uint32_t, impl::PointKeyHash> m_edges;

  explicit TinView(TileMesh const & mesh, measurement_utils::Units units) : m_mesh(mesh)
  {
    m_altitudes = mesh.GetAltitudes();
    if (units == measurement_utils::Units::Imperial)
    {
      m_feet.reserve(mesh.GetAltitudes().size());
      for (int32_t altitude : mesh.GetAltitudes())
        m_feet.push_back(DisplayAltitude(altitude, units));
      m_altitudes = m_feet;
    }
    m_edges.reserve(mesh.GetTriangles().size());
    for (uint32_t t = 0; t < Count(); ++t)
    {
      auto const v = Triangle(t);
      for (size_t e = 0; e < 3; ++e)
        VERIFY(m_edges.emplace(Edge(v[e], v[(e + 1) % 3]), t).second, ());
    }
  }
  static uint64_t Edge(uint32_t u, uint32_t v) { return (uint64_t{u} << 32) | v; }
  uint32_t Count() const { return static_cast<uint32_t>(m_mesh.GetTrianglesCount()); }
  std::array<uint32_t, 3> Triangle(uint32_t t) const
  {
    auto const & indices = m_mesh.GetTriangles();
    return {indices[3 * t], indices[3 * t + 1], indices[3 * t + 2]};
  }
  uint32_t Neighbor(uint32_t t, uint8_t edge) const
  {
    auto const v = Triangle(t);
    auto const it = m_edges.find(Edge(v[(edge + 1) % 3], v[edge]));
    return it == m_edges.end() ? kNone : it->second;
  }
  m2::PointU PointU(uint32_t v) const { return m_mesh.GetPointU(v); }
  m2::PointD Point(uint32_t v) const { return m_mesh.GetPoints()[v]; }
};

struct CrossedEdges
{
  uint8_t m_entry = 3, m_exit = 3;
};

template <typename View>
CrossedEdges GetCrossedEdges(View const & view, uint32_t triangle, int32_t level)
{
  CrossedEdges result;
  auto const v = view.Triangle(triangle);
  for (uint8_t e = 0; e < 3; ++e)
  {
    bool const a = view.m_altitudes[v[e]] >= level;
    bool const b = view.m_altitudes[v[(e + 1) % 3]] >= level;
    if (a != b)
      (a ? result.m_exit : result.m_entry) = e;
  }
  ASSERT(result.m_entry != 3 && result.m_exit != 3, (triangle, level));
  return result;
}

template <typename View>
EdgeLevel Key(View const & view, uint32_t triangle, uint8_t edge, int32_t level)
{
  auto const v = view.Triangle(triangle);
  uint64_t a = impl::PointKey(view.PointU(v[edge]));
  uint64_t b = impl::PointKey(view.PointU(v[(edge + 1) % 3]));
  if (b < a)
    std::swap(a, b);
  return {a, b, level};
}

template <typename View>
m2::PointD Crossing(View const & view, uint32_t triangle, uint8_t edge, int32_t level)
{
  auto const v = view.Triangle(triangle);
  uint32_t a = v[edge], b = v[(edge + 1) % 3];
  // Both representations use physical endpoint ordering, independently of local IDs.
  if (impl::PointKey(view.PointU(b)) < impl::PointKey(view.PointU(a)))
    std::swap(a, b);
  double const t = (static_cast<double>(level) - view.m_altitudes[a]) /
                   (static_cast<double>(view.m_altitudes[b]) - view.m_altitudes[a]);
  return view.Point(a) + (view.Point(b) - view.Point(a)) * t;
}

void AppendPoint(std::vector<m2::PointD> & points, m2::PointD const & point)
{
  if (points.empty() || points.back() != point)
    points.push_back(point);
}

template <typename View, typename Emit>
void TraceView(View const & view, int32_t step, Emit const & emit, base::SharedCancellable const & isCancellable)
{
  uint32_t const count = view.Count();
  if (count == 0)
    return;
  int32_t first = std::numeric_limits<int32_t>::max(), last = std::numeric_limits<int32_t>::min();
  std::vector<std::pair<int32_t, int32_t>> ranges(count);
  for (uint32_t t = 0; t < count; ++t)
  {
    auto const v = view.Triangle(t);
    auto const [minimum, maximum] =
        std::minmax({view.m_altitudes[v[0]], view.m_altitudes[v[1]], view.m_altitudes[v[2]]});
    int32_t const lo = math::FloorDiv(minimum, step) + 1, hi = math::FloorDiv(maximum, step);
    ranges[t] = {lo, hi};
    if (lo <= hi)
    {
      first = std::min(first, lo);
      last = std::max(last, hi);
    }
  }
  if (first > last)
    return;
  // The int16 meters span ~10.7k levels even in feet at the finest 20 ft step.
  CHECK_LESS(last - first, 1 << 16, (first, last, step));
  std::vector<std::vector<uint32_t>> bins(last - first + 1);
  for (uint32_t t = 0; t < count; ++t)
    for (int32_t l = ranges[t].first; l <= ranges[t].second; ++l)
      bins[l - first].push_back(t);
  std::vector<uint32_t> visited(count, 0);
  for (size_t bin = 0; bin < bins.size(); ++bin)
  {
    if (isCancellable.IsCancelled())
      return;
    int32_t const level = (first + static_cast<int32_t>(bin)) * step;
    uint32_t const epoch = static_cast<uint32_t>(bin) + 1;
    for (uint32_t const seed : bins[bin])
    {
      if (visited[seed] == epoch)
        continue;
      uint32_t start = seed;
      bool closed = false;
      for (uint32_t guard = 0; guard <= count; ++guard)
      {
        auto const edges = GetCrossedEdges(view, start, level);
        uint32_t const previous = view.Neighbor(start, edges.m_entry);
        if (previous == kNone)
          break;
        if (previous == seed)
        {
          closed = true;
          start = seed;
          break;
        }
        start = previous;
        ASSERT_LESS(guard, count, ("Unterminated contour backward walk", level));
      }
      Fragment fragment;
      auto & line = fragment.m_line;
      line.m_altitude = level;
      line.m_closed = closed;
      uint32_t current = start;
      for (uint32_t guard = 0; guard <= count; ++guard)
      {
        auto const edges = GetCrossedEdges(view, current, level);
        if (line.m_points.empty() && !closed)
        {
          fragment.m_entry = Key(view, current, edges.m_entry, level);
          AppendPoint(line.m_points, Crossing(view, current, edges.m_entry, level));
        }
        AppendPoint(line.m_points, Crossing(view, current, edges.m_exit, level));
        visited[current] = epoch;
        uint32_t const next = view.Neighbor(current, edges.m_exit);
        if (next == kNone)
        {
          fragment.m_exit = Key(view, current, edges.m_exit, level);
          break;
        }
        if (next == start && closed)
        {
          AppendPoint(line.m_points, line.m_points.front());
          break;
        }
        current = next;
        ASSERT_LESS(guard, count, ("Unterminated contour forward walk", level));
      }
      // Retain one-point open fragments: different physical edges can cross at
      // the same vertex, and the fragment still connects its two interfaces.
      emit(std::move(fragment));
    }
  }
}

void EmitLine(Isoline && line, IsolineFn const & fn)
{
  if (line.m_points.size() >= (line.m_closed ? 4u : 2u))
    fn(std::move(line));
}
}  // namespace

void TraceIsolines(TileMesh const & mesh, int32_t step, measurement_utils::Units units, IsolineFn const & fn,
                   base::SharedCancellable const & isCancellable)
{
  ASSERT_GREATER(step, 0, ());
  TraceView(TinView(mesh, units), step, [&](Fragment && fragment) { EmitLine(std::move(fragment.m_line), fn); },
            isCancellable);
}

void TraceIsolines(TerrainData const & data, int32_t step, measurement_utils::Units units, IsolineFn const & fn,
                   base::SharedCancellable const & isCancellable)
{
  TraceIsolines(data.m_mesh, step, units, fn, isCancellable);
}
}  // namespace terrain
