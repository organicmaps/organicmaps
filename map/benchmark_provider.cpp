#include "base/SRC_FIRST.hpp"
#include "map/benchmark_provider.hpp"
#include "geometry/mercator.hpp"

BenchmarkRectProvider::BenchmarkRectProvider(int startLevel, m2::RectD const & startRect, int endLevel)
  : m_endLevel(endLevel)
{
  m_rects.push_back(make_pair(startRect, startLevel));
}

bool BenchmarkRectProvider::hasRect() const
{
  return !m_rects.empty();
}

m2::RectD const BenchmarkRectProvider::nextRect()
{
  pair<m2::RectD, int> const & f = m_rects.front();
  m2::RectD r = f.first;

  if (f.second < m_endLevel)
  {
    int nextLevel = f.second + 1;

    m_rects.push_back(make_pair(m2::RectD(r.minX(), r.minY(), r.minX() + r.SizeX() / 2, r.minY() + r.SizeY() / 2), nextLevel));
    m_rects.push_back(make_pair(m2::RectD(r.minX() + r.SizeX() / 2, r.minY(), r.maxX(), r.minY() + r.SizeY() / 2), nextLevel));
    m_rects.push_back(make_pair(m2::RectD(r.minX(), r.minY() + r.SizeY() / 2, r.minX() + r.SizeX() / 2, r.maxY()), nextLevel));
    m_rects.push_back(make_pair(m2::RectD(r.minX() + r.SizeX() / 2, r.minY() + r.SizeY() / 2, r.maxX(), r.maxY()), nextLevel));
  }

  m_rects.pop_front();
  return r;
}
