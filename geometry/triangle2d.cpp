#include "triangle2d.hpp"

#include "geometry/parametrized_segment.hpp"
#include "geometry/robust_orientation.hpp"
#include "geometry/segment2d.hpp"

#include "base/math.hpp"

#include <chrono>
#include <limits>
#include <random>

using namespace m2::robust;
using namespace std;

namespace m2
{
bool IsPointInsideTriangle(m2::PointD const & pt, m2::PointD const & p1,
                           m2::PointD const & p2, m2::PointD const & p3)
{
  double const s1 = OrientedS(p1, p2, pt);
  double const s2 = OrientedS(p2, p3, pt);
  double const s3 = OrientedS(p3, p1, pt);

  // In the case of degenerate triangles we need to check that pt lies
  // on (p1, p2), (p2, p3) or (p3, p1).
  if (s1 == 0.0 && s2 == 0.0 && s3 == 0.0)
  {
    return IsPointOnSegment(pt, p1, p2) || IsPointOnSegment(pt, p2, p3) ||
           IsPointOnSegment(pt, p3, p1);
  }

  return ((s1 >= 0.0 && s2 >= 0.0 && s3 >= 0.0) ||
          (s1 <= 0.0 && s2 <= 0.0 && s3 <= 0.0));
}

bool IsPointStrictlyInsideTriangle(m2::PointD const & pt, m2::PointD const & p1,
                                   m2::PointD const & p2, m2::PointD const & p3)
{
  double const s1 = OrientedS(p1, p2, pt);
  double const s2 = OrientedS(p2, p3, pt);
  double const s3 = OrientedS(p3, p1, pt);

  return ((s1 > 0.0 && s2 > 0.0 && s3 > 0.0) ||
          (s1 < 0.0 && s2 < 0.0 && s3 < 0.0));
}

bool IsPointInsideTriangles(m2::PointD const & pt, vector<m2::TriangleD> const & v)
{
  for (auto const & triangle : v)
  {
    if (IsPointInsideTriangle(pt, triangle.p1(), triangle.p2(), triangle.p3()))
      return true;
  }
  return false;
}

m2::PointD GetRandomPointInsideTriangle(m2::TriangleD const & t)
{
  size_t kDistribMax = 1000;

  auto const seed = static_cast<uint32_t>(chrono::system_clock::now().time_since_epoch().count());
  default_random_engine engine(seed);
  uniform_int_distribution<size_t> distrib(0, kDistribMax);
  double const r1 = sqrt(static_cast<double>(distrib(engine)) / kDistribMax);
  double const r2 = static_cast<double>(distrib(engine)) / kDistribMax;
  return t.m_points[0] * (1.0 - r1) + t.m_points[1] * r1 * (1.0 - r2) + t.m_points[2] * r2 * r1;
}

m2::PointD GetRandomPointInsideTriangles(vector<m2::TriangleD> const & v)
{
  if (v.empty())
    return m2::PointD();

  auto const seed = static_cast<uint32_t>(chrono::system_clock::now().time_since_epoch().count());
  default_random_engine engine(seed);
  uniform_int_distribution<size_t> distrib(0, v.size() - 1);
  return GetRandomPointInsideTriangle(v[distrib(engine)]);
}

m2::PointD ProjectPointToTriangles(m2::PointD const & pt, vector<m2::TriangleD> const & v)
{
  if (v.empty())
    return pt;

  int minT = -1;
  int minI = -1;
  double minDist = numeric_limits<double>::max();
  for (int t = 0; t < static_cast<int>(v.size()); t++)
  {
    for (int i = 0; i < 3; i++)
    {
      m2::ParametrizedSegment<m2::PointD> segment(v[t].m_points[i], v[t].m_points[(i + 1) % 3]);
      double const dist = segment.SquaredDistanceToPoint(pt);
      if (dist < minDist)
      {
        minDist = dist;
        minT = t;
        minI = i;
      }
    }
  }
  m2::ParametrizedSegment<m2::PointD> segment(v[minT].m_points[minI],
                                              v[minT].m_points[(minI + 1) % 3]);
  return segment.ClosestPointTo(pt);
}
} // namespace m2;
