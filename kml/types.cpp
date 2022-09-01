#include "kml/types.hpp"

#include "kml/minzoom_quadtree.hpp"

#include "base/macros.hpp"
#include "base/string_utils.hpp"

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
  auto const setMinZoom = [](ValueType & valueType, int minZoom)
  {
    valueType.first->m_minZoom = minZoom;
  };
  minZoomQuadtree.SetMinZoom(countPerTile, maxZoom, setMinZoom);
}
}  // namespace kml
