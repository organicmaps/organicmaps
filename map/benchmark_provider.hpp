#pragma once

#include "geometry/rect2d.hpp"
#include "std/list.hpp"

struct BenchmarkRectProvider
{
  int m_endLevel;
  list<pair<m2::RectD, int> > m_rects;
  BenchmarkRectProvider(int startLevel, m2::RectD const & startRect, int endLevel);

  bool hasRect() const;
  m2::RectD const nextRect();
};

