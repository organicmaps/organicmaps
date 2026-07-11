#include "indexer/terrain/twm_grid.hpp"

#include "indexer/terrain/terrain_utils.hpp"

#include "geometry/mercator.hpp"

#include "base/exception.hpp"
#include "base/string_utils.hpp"

#include <fstream>
#include <sstream>

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

std::vector<GridBlock> LoadTwmGrid(std::string const & filePath)
{
  std::ifstream file(filePath);
  if (!file)
    MYTHROW(RootException, ("Can't open the TWM grid index", filePath));
  std::stringstream buffer;
  buffer << file.rdbuf();
  return ParseTwmGrid(buffer.str(), filePath);
}

std::vector<GridBlock> ParseTwmGrid(std::string const & content, std::string const & sourceName)
{
  std::istringstream file(content);

  std::vector<GridBlock> blocks;
  std::string line;
  while (std::getline(file, line))
  {
    strings::Trim(line);
    if (line.empty() || line.front() == '#')
      continue;

    // "N35E070 5 5": the bottom-left corner name, the width and the height in degrees.
    strings::SimpleTokenizer it(line, " \t");
    GridBlock block;
    int latSign = 0;
    int lonSign = 0;
    if (it)
    {
      std::string_view const name = *it;
      if (name.size() == 7 && (name[0] == 'N' || name[0] == 'S') && (name[3] == 'E' || name[3] == 'W'))
      {
        latSign = name[0] == 'N' ? 1 : -1;
        lonSign = name[3] == 'E' ? 1 : -1;
        int lat;
        int lon;
        if (strings::to_int(name.substr(1, 2), lat) && strings::to_int(name.substr(4, 3), lon))
        {
          block.m_bottom = latSign * lat;
          block.m_left = lonSign * lon;
        }
        else
        {
          latSign = 0;
        }
      }
    }
    if (latSign == 0 || lonSign == 0 || !++it || !strings::to_int(*it, block.m_width) || !++it ||
        !strings::to_int(*it, block.m_height))
    {
      MYTHROW(RootException, ("Malformed TWM grid line", line, "in", sourceName));
    }

    if (block.m_width <= 0 || block.m_height <= 0 || block.m_left < -180 || block.m_left + block.m_width > 180 ||
        block.m_bottom < -85 || block.m_bottom + block.m_height > 85)
    {
      MYTHROW(RootException, ("Invalid TWM grid block", line, "in", sourceName));
    }
    blocks.push_back(block);
  }

  if (blocks.empty())
    MYTHROW(RootException, ("Empty TWM grid index", sourceName));
  return blocks;
}
}  // namespace terrain
