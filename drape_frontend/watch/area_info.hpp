#pragma once

#include "geometry/point2d.hpp"

#include "std/vector.hpp"
#include "std/algorithm.hpp"

namespace df
{
namespace watch
{

class AreaInfo
{
  m2::PointD m_center;

public:
  vector<m2::PointD> m_path;

  void reserve(size_t ptsCount)
  {
    m_path.reserve(ptsCount);
  }

  void swap(AreaInfo & r)
  {
    m_path.swap(r.m_path);
    std::swap(m_center, r.m_center);
  }

  void push_back(m2::PointD const & pt)
  {
    m_path.push_back(pt);
  }

  size_t size() const { return m_path.size(); }

  void SetCenter(m2::PointD const & p) { m_center = p; }
  m2::PointD GetCenter() const { return m_center; }
};

}
}

inline void swap(df::watch::AreaInfo & p1, df::watch::AreaInfo & p2)
{
  p1.swap(p2);
}
