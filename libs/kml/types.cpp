#include "kml/types.hpp"

#include "kml/minzoom_quadtree.hpp"

#include "base/macros.hpp"
#include "base/random.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include <algorithm>
#include <random>
#include <utility>

namespace kml
{
void SetBookmarksMinZoom(FileData & fileData, double countPerTile, int maxZoom)
{
  using ValueType = std::pair<BookmarkData *, int /* score */>;
  auto const scoreLess = base::LessBy(&ValueType::second);
  MinZoomQuadtree<ValueType, decltype(scoreLess)> minZoomQuadtree{scoreLess};
  for (auto & bm : fileData.m_bookmarksData)
  {
    auto const & properties = bm.m_properties;
    int score = 0;
    if (auto const s = properties.find("score"); s != properties.end())
      UNUSED_VALUE(strings::to_int(s->second, score));
    minZoomQuadtree.Add(bm.m_point, ValueType{&bm, score});
  }
  auto const setMinZoom = [](ValueType & valueType, int minZoom) { valueType.first->m_minZoom = minZoom; };
  minZoomQuadtree.SetMinZoom(countPerTile, maxZoom, setMinZoom);
}

bool MultiGeometry::IsValidSectionTimestamps(TimeT const & times, size_t pointCount)
{
  return times.empty() || (times.size() == pointCount && std::ranges::is_sorted(times) &&
                           std::ranges::find(times, base::INVALID_TIME_STAMP) == times.end());
}

void MultiGeometry::RemoveDuplicatePoints(LineT & line, TimeT & times)
{
  bool const timed = !times.empty();
  ASSERT(!timed || times.size() == line.size(), (line.size(), times.size()));

  size_t kept = 0;
  for (size_t i = 0; i < line.size(); ++i)
  {
    if (kept != 0 && IsSameTrackPoint(line[kept - 1].GetPoint(), line[i].GetPoint()))
    {
      if (line[kept - 1].GetAltitude() == geometry::kInvalidAltitude)
        line[kept - 1].SetAltitude(line[i].GetAltitude());
      if (timed && times[kept - 1] == base::INVALID_TIME_STAMP)
        times[kept - 1] = times[i];
      continue;
    }
    line[kept] = line[i];
    if (timed)
      times[kept] = times[i];
    ++kept;
  }
  line.resize(kept);
  if (timed)
    times.resize(kept);
}

bool MultiGeometry::IsValid() const
{
  if (!HasLines() || m_lines.size() != m_timestamps.size())
    return false;

  for (size_t i = 0; i < m_lines.size(); ++i)
    if (m_lines[i].size() < 2 || !IsValidSectionTimestamps(m_timestamps[i], m_lines[i].size()))
      return false;
  return true;
}

void MultiGeometry::Clear()
{
  m_lines.clear();
  m_timestamps.clear();
}

bool MultiGeometry::HasTimestamps() const
{
  return std::ranges::any_of(m_timestamps, [](auto const & times) { return !times.empty(); });
}

void MultiGeometry::FromPoints(std::vector<m2::PointD> const & points)
{
  LineT line;
  line.reserve(points.size());
  for (auto const & pt : points)
    line.emplace_back(pt, geometry::kInvalidAltitude);

  ASSERT(line.size() > 1, ());
  m_lines.push_back(std::move(line));
  // Every line has a timestamp vector, even when no point has a capture time.
  m_timestamps.emplace_back();
}

void MultiGeometry::AddLine(std::initializer_list<geometry::PointWithAltitude> lst)
{
  m_lines.emplace_back();
  m_lines.back().assign(lst);
}

void MultiGeometry::AddTimestamps(std::initializer_list<TimeInt> lst)
{
  m_timestamps.emplace_back();
  m_timestamps.back().assign(lst);
}

MultiGeometry mergeGeometry(std::vector<MultiGeometry> && aGeometries)
{
  MultiGeometry merged;
  for (auto && geometry : aGeometries)
  {
    for (auto && line : geometry.m_lines)
      merged.m_lines.push_back(std::move(line));
    for (auto && ts : geometry.m_timestamps)
      merged.m_timestamps.push_back(std::move(ts));
  }

  return merged;
}

PredefinedColor GetRandomPredefinedColor()
{
  thread_local base::UniformRandom<uint8_t> distr(1, std::to_underlying(PredefinedColor::Count) - 1);
  return static_cast<PredefinedColor>(distr());
}

}  // namespace kml
