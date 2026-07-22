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
    bool nameOk = it && ParseBlockName(*it, block.m_bottom, block.m_left);
    if (!nameOk || !++it || !strings::to_int(*it, block.m_width) || !++it || !strings::to_int(*it, block.m_height))
      MYTHROW(RootException, ("Malformed TWM grid line", line, "in", sourceName));

    if (!IsValidBlock(block))
      MYTHROW(RootException, ("Invalid TWM grid block", line, "in", sourceName));
    blocks.push_back(block);
  }

  if (blocks.empty())
    MYTHROW(RootException, ("Empty TWM grid index", sourceName));
  return blocks;
}
}  // namespace terrain
