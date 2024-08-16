#include "kml/types.hpp"

#include "kml/minzoom_quadtree.hpp"

#include "base/macros.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <random>

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
  auto const setMinZoom = [](ValueType & valueType, int minZoom)
  {
    valueType.first->m_minZoom = minZoom;
  };
  minZoomQuadtree.SetMinZoom(countPerTile, maxZoom, setMinZoom);
}

void MultiGeometry::FromPoints(std::vector<m2::PointD> const & points)
{
  LineT line;
  for (auto const & pt : points)
    line.emplace_back(pt);

  ASSERT(line.size() > 1, ());
  m_lines.push_back(std::move(line));
}

MultiGeometry mergeGeometry(std::vector<MultiGeometry> && aGeometries)
{
  MultiGeometry merged;
  for (auto && geometry : aGeometries)
    for (auto && line : geometry.m_lines)
      merged.m_lines.push_back(std::move(line));

  return merged;
}

kml::PredefinedColor GetRandomPredefinedColor()
{
  // Simple time-based seed instead of random_device is enough.
  static std::mt19937 gen(static_cast<uint8_t>(std::chrono::system_clock::now().time_since_epoch().count()));
  static std::uniform_int_distribution<> distr(1, static_cast<uint8_t>(PredefinedColor::Count) - 1);
  return static_cast<PredefinedColor>(distr(gen));
}
}  // namespace kml
