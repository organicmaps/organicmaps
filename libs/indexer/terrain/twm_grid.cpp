#include "indexer/terrain/twm_grid.hpp"

#include "indexer/terrain/terrain_utils.hpp"

#include "geometry/mercator.hpp"

#include "base/string_utils.hpp"

namespace terrain
{
std::string GridBlock::GetFileName() const
{
  return GetBlockFileName(m_bottom, m_left);
}

m2::RectD GridBlock::GetRectMercator() const
{
  return {mercator::FromLatLon(m_bottom, m_left), mercator::FromLatLon(m_bottom + m_height, m_left + m_width)};
}

bool ParseBlockName(std::string_view name, int & bottom, int & left)
{
  if (name.size() != 7 || (name[0] != 'N' && name[0] != 'S') || (name[3] != 'E' && name[3] != 'W'))
    return false;
  int lat;
  int lon;
  if (!strings::to_int(name.substr(1, 2), lat) || !strings::to_int(name.substr(4, 3), lon))
    return false;
  bottom = (name[0] == 'N' ? 1 : -1) * lat;
  left = (name[3] == 'E' ? 1 : -1) * lon;
  return true;
}

bool IsValidBlock(GridBlock const & block)
{
  return block.m_width > 0 && block.m_height > 0 && block.m_left >= -180 && block.m_left + block.m_width <= 180 &&
         block.m_bottom >= -85 && block.m_bottom + block.m_height <= 85;
}
}  // namespace terrain
