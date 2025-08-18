#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace generator
{
namespace cells_merger
{
struct CellWrapper
{
  static CellWrapper const kEmpty;

  CellWrapper() = default;
  CellWrapper(m2::RectD const & cell) : m_cell(cell) {}

  m2::RectD const & GetCell() const { return m_cell; }
  void SetTopLeft(size_t val) { m_topLeft = val; }
  size_t GetTopLeft() const { return m_topLeft; }
  void SetTopRight(size_t val) { m_topRight = val; }
  size_t GetTopRight() const { return m_topRight; }
  void SetBottomLeft(size_t val) { m_bottomLeft = val; }
  size_t GetBottomLeft() const { return m_bottomLeft; }
  void SetBottomRight(size_t val) { m_bottomRight = val; }
  size_t GetBottomRight() const { return m_bottomRight; }
  void CalcSum() { m_sum = m_topLeft + m_topRight + m_bottomLeft + m_bottomRight; }
  size_t GetSum() const { return m_sum; }

private:
  m2::RectD m_cell;
  size_t m_topLeft = 0;
  size_t m_topRight = 0;
  size_t m_bottomLeft = 0;
  size_t m_bottomRight = 0;
  size_t m_sum = 0;
};

// CellsMerger combines square cells to large square cells, if possible. This is necessary to reduce
// the number of cells. For example, later they can be used to build the index.
// Important: input cells are disjoint squares that have the same size.
// The algorithm does not guarantee that the optimal solution will be found.
class CellsMerger
{
public:
  explicit CellsMerger(std::vector<m2::RectD> && cells);

  std::vector<m2::RectD> Merge();

private:
  bool Has(int32_t x, int32_t y) const;
  bool Has(m2::PointI const & xy) const;
  CellWrapper & Get(m2::PointI const & xy);
  CellWrapper & Get(int32_t x, int32_t y);
  CellWrapper const & TryGet(int32_t x, int32_t y, CellWrapper const & defaultValue = CellWrapper::kEmpty) const;

  void CalcSum();
  std::optional<m2::PointI> FindMax() const;
  std::optional<m2::PointI> FindDirection(m2::PointI const & startXy) const;
  m2::PointI FindBigSquare(m2::PointI const & xy, m2::PointI const & direction) const;
  m2::RectD Union(m2::PointI const & startXy);
  void Remove(m2::PointI const & minXy, m2::PointI const & maxXy);

  std::map<m2::PointI, CellWrapper> m_matrix;
  int32_t m_maxX = 0;
  int32_t m_maxY = 0;
};

std::vector<m2::RectD> MakeNet(double step, double minX, double minY, double maxX, double maxY);
}  // namespace cells_merger
}  // namespace generator
