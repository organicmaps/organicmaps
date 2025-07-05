#include "generator/cells_merger.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <cmath>

namespace
{
double GetMinX(std::vector<m2::RectD> const & cells)
{
  CHECK(!cells.empty(), ());
  auto const minElementX =
      std::min_element(std::cbegin(cells), std::cend(cells),
                       [](auto const & l, auto const & r) { return l.minX() < r.minX(); });
  return minElementX->minX();
}

double GetMinY(std::vector<m2::RectD> const & cells)
{
  CHECK(!cells.empty(), ());
  auto const minElementY =
      std::min_element(std::cbegin(cells), std::cend(cells),
                       [](auto const & l, auto const & r) { return l.minY() < r.minY(); });
  return minElementY->minY();
}
}  // namespace

namespace generator
{
namespace cells_merger
{
// static
CellWrapper const CellWrapper::kEmpty;

CellsMerger::CellsMerger(std::vector<m2::RectD> && cells)
{
  if (cells.empty())
    return;
  auto const minX = GetMinX(cells);
  auto const minY = GetMinY(cells);
  auto const sideSize = cells.front().SizeX();
  for (auto const & cell : cells)
  {
    int32_t const xIdx = std::lround((cell.minX() - minX) / sideSize);
    int32_t const yIdx = std::lround((cell.minY() - minY) / sideSize);
    m_matrix.emplace(m2::PointI{xIdx, yIdx}, cell);
    m_maxX = std::max(m_maxX, xIdx);
    m_maxY = std::max(m_maxY, yIdx);
  }
}

std::vector<m2::RectD> CellsMerger::Merge()
{
  CalcSum();
  std::vector<m2::RectD> cells;
  while (auto const max = FindMax())
    cells.emplace_back(Union(*max));
  return cells;
}

void CellsMerger::CalcSum()
{
  // Bottom left
  for (int32_t x = 0; x <= m_maxX; ++x)
  {
    for (int32_t y = 0; y <= m_maxY; ++y)
    {
      if (!Has(x, y))
        continue;
      auto & cell = Get(x, y);
      cell.SetBottomLeft(
          std::min({TryGet(x - 1, y).GetBottomLeft(), TryGet(x, y - 1).GetBottomLeft(),
                    TryGet(x - 1, y - 1).GetBottomLeft()}) +
          1);
    }
  }
  // Bottom right
  for (int32_t x = m_maxX; x >= 0; --x)
  {
    for (int32_t y = 0; y <= m_maxY; ++y)
    {
      if (!Has(x, y))
        continue;
      auto & cell = Get(x, y);
      cell.SetBottomRight(
          std::min({TryGet(x + 1, y).GetBottomRight(), TryGet(x, y - 1).GetBottomRight(),
                    TryGet(x + 1, y - 1).GetBottomRight()}) +
          1);
    }
  }
  // Top left
  for (int32_t x = 0; x <= m_maxX; ++x)
  {
    for (int32_t y = m_maxY; y >= 0; --y)
    {
      if (!Has(x, y))
        continue;
      auto & cell = Get(x, y);
      cell.SetTopLeft(std::min({TryGet(x - 1, y).GetTopLeft(), TryGet(x, y + 1).GetTopLeft(),
                                TryGet(x - 1, y + 1).GetTopLeft()}) +
                      1);
    }
  }
  // Top right
  for (int32_t x = m_maxX; x >= 0; --x)
  {
    for (int32_t y = m_maxY; y >= 0; --y)
    {
      if (!Has(x, y))
        continue;
      auto & cell = Get(x, y);
      cell.SetTopRight(std::min({TryGet(x + 1, y).GetTopRight(), TryGet(x, y + 1).GetTopRight(),
                                 TryGet(x + 1, y + 1).GetTopRight()}) +
                       1);
    }
  }
  for (auto & pair : m_matrix)
    pair.second.CalcSum();
}

CellWrapper & CellsMerger::Get(m2::PointI const & xy)
{
  auto it = m_matrix.find(xy);
  CHECK(it != std::cend(m_matrix), ());
  return it->second;
}

CellWrapper & CellsMerger::Get(int32_t x, int32_t y) { return Get({x, y}); }

CellWrapper const & CellsMerger::TryGet(int32_t x, int32_t y,
                                        CellWrapper const & defaultValue) const
{
  auto const it = m_matrix.find({x, y});
  return it == std::cend(m_matrix) ? defaultValue : it->second;
}

m2::PointI CellsMerger::FindBigSquare(m2::PointI const & xy, m2::PointI const & direction) const
{
  m2::PointI prevXy = xy;
  auto currXy = prevXy + direction;
  while (true)
  {
    auto const xMinMax = std::minmax(currXy.x, xy.x);
    for (int32_t x = xMinMax.first; x <= xMinMax.second; ++x)
    {
      if (!Has({x, currXy.y}))
        return prevXy;
    }
    auto const yMinMax = std::minmax(currXy.y, xy.y);
    for (int32_t y = yMinMax.first; y <= yMinMax.second; ++y)
    {
      if (!Has({currXy.x, y}))
        return prevXy;
    }
    prevXy = currXy;
    currXy += direction;
  }
}

std::optional<m2::PointI> CellsMerger::FindDirection(m2::PointI const & startXy) const
{
  std::array<std::pair<size_t, m2::PointI>, 4> directionsWithWeight;
  std::array<m2::PointI, 4> const directions{{{1, 1}, {-1, 1}, {1, -1}, {-1, -1}}};
  base::Transform(directions, std::begin(directionsWithWeight),
                  [&](auto const & direction) {
                    return std::make_pair(
                        TryGet(startXy.x + direction.x, startXy.y).GetSum() +
                            TryGet(startXy.x, startXy.y + direction.y).GetSum() +
                            TryGet(startXy.x + direction.x, startXy.y + direction.y).GetSum(),
                        direction);
                 });
  auto const direction =
      std::max_element(std::cbegin(directionsWithWeight), std::cend(directionsWithWeight))->second;
  return Has(startXy + direction) ? direction : std::optional<m2::PointI>{};
}

void CellsMerger::Remove(m2::PointI const & minXy, m2::PointI const & maxXy)
{
  auto const xMinMax = std::minmax(minXy.x, maxXy.x);
  auto const yMinMax = std::minmax(minXy.y, maxXy.y);
  for (int32_t x = xMinMax.first; x <= xMinMax.second; ++x)
  {
    for (int32_t y = yMinMax.first; y <= yMinMax.second; ++y)
      m_matrix.erase({x, y});
  }
}

bool CellsMerger::Has(int32_t x, int32_t y) const { return m_matrix.count({x, y}) != 0; }

bool CellsMerger::Has(const m2::PointI & xy) const { return Has(xy.x, xy.y); }

std::optional<m2::PointI> CellsMerger::FindMax() const
{
  m2::PointI max;
  size_t sum = 0;
  for (auto const & pair : m_matrix)
  {
    auto const cellSum = pair.second.GetSum();
    if (cellSum > sum)
    {
      sum = cellSum;
      max = pair.first;
    }
  }
  return sum != 0 ? max : std::optional<m2::PointI>{};
}

m2::RectD CellsMerger::Union(m2::PointI const & startXy)
{
  auto const direction = FindDirection(startXy);
  if (!direction)
  {
    auto const united = Get(startXy).GetCell();
    Remove(startXy, startXy);
    return united;
  }
  auto nextXy = FindBigSquare(startXy, *direction);
  auto cell1 = Get(startXy).GetCell();
  auto const & cell2 = Get(nextXy).GetCell();
  cell1.Add(cell2);
  Remove(startXy, nextXy);
  return cell1;
}

std::vector<m2::RectD> MakeNet(double step, double minX, double minY, double maxX, double maxY)
{
  CHECK_LESS(minX, maxX, ());
  CHECK_LESS(minY, maxY, ());
  std::vector<m2::RectD> net;
  auto xmin = minX;
  auto ymin = minY;
  auto x = xmin;
  auto y = ymin;
  while (xmin < maxX)
  {
    x += step;
    while (ymin < maxY)
    {
      y += step;
      net.emplace_back(m2::RectD{{xmin, ymin}, {x, y}});
      ymin = y;
    }
    ymin = minY;
    y = ymin;
    xmin = x;
  }
  return net;
}
}  // namespace cells_merger
}  // namespace generator
