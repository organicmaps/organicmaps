#include "indexer/feature_algo.hpp"
#include "indexer/feature.hpp"

#include "base/logging.hpp"


namespace feature
{

class CalculateLineCenter
{
  typedef m2::PointD P;

  struct Value
  {
    Value(P const & p, double l) : m_p(p), m_len(l) {}

    bool operator< (Value const & r) const { return (m_len < r.m_len); }

    P m_p;
    double m_len;
  };

  vector<Value> m_poly;
  double m_length;

public:
  CalculateLineCenter() : m_length(0.0) {}

  void operator() (m2::PointD const & pt)
  {
    m_length += (m_poly.empty() ? 0.0 : m_poly.back().m_p.Length(pt));
    m_poly.emplace_back(pt, m_length);
  }

  P GetCenter() const
  {
    typedef vector<Value>::const_iterator IterT;

    double const l = m_length / 2.0;

    IterT e = lower_bound(m_poly.begin(), m_poly.end(), Value(m2::PointD(0, 0), l));
    if (e == m_poly.begin())
    {
      /// @todo It's very strange, but we have linear objects with zero length.
      LOG(LWARNING, ("Zero length linear object"));
      return e->m_p;
    }

    IterT b = e-1;

    double const f = (l - b->m_len) / (e->m_len - b->m_len);

    // For safety reasons (floating point calculations) do comparison instead of ASSERT.
    if (0.0 <= f && f <= 1.0)
      return (b->m_p * (1-f) + e->m_p * f);
    else
      return ((b->m_p + e->m_p) / 2.0);
  }
};

class CalculatePointOnSurface
{
  typedef m2::PointD P;
  P m_center;
  P m_rectCenter;
  double m_squareDistanceToApproximate;

public:
  CalculatePointOnSurface(m2::RectD const & rect)
  {
    m_rectCenter = rect.Center();
    m_center = m_rectCenter;
    m_squareDistanceToApproximate = numeric_limits<double>::max();
  }

  void operator() (P const & p1, P const & p2, P const & p3)
  {
    if (m_squareDistanceToApproximate == 0.0)
      return;
    if (m2::IsPointInsideTriangle(m_rectCenter, p1, p2, p3))
    {
      m_center = m_rectCenter;
      m_squareDistanceToApproximate = 0.0;
      return;
    }
    P triangleCenter(p1);
    triangleCenter += p2;
    triangleCenter += p3;
    triangleCenter = triangleCenter / 3.0;

    double triangleDistance = m_rectCenter.SquareLength(triangleCenter);
    if (triangleDistance <= m_squareDistanceToApproximate)
    {
      m_center = triangleCenter;
      m_squareDistanceToApproximate = triangleDistance;
    }
  }
  P GetCenter() const { return m_center; }
};

m2::PointD GetCenter(FeatureType const & f, int scale)
{
  feature::EGeomType const type = f.GetFeatureType();
  switch (type)
  {
  case feature::GEOM_POINT:
    return f.GetCenter();

  case feature::GEOM_LINE:
    {
      CalculateLineCenter doCalc;
      f.ForEachPointRef(doCalc, scale);
      return doCalc.GetCenter();
    }

  default:
    {
      ASSERT_EQUAL ( type, feature::GEOM_AREA, () );
      CalculatePointOnSurface doCalc(f.GetLimitRect(scale));
      f.ForEachTriangleRef(doCalc, scale);
      return doCalc.GetCenter();
    }
  }
}

m2::PointD GetCenter(FeatureType const & f) { return GetCenter(f, FeatureType::BEST_GEOMETRY); }

}
