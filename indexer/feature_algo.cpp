#include "feature_algo.hpp"
#include "feature.hpp"


namespace feature
{

class CalcPolyCenter
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
  CalcPolyCenter() : m_length(0.0) {}

  void operator() (CoordPointT const & pt)
  {
    m2::PointD p(pt.first, pt.second);

    m_length += (m_poly.empty() ? 0.0 : m_poly.back().m_p.Length(p));
    m_poly.push_back(Value(p, m_length));
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

class CalcMassCenter
{
  typedef m2::PointD P;
  P m_center;
  size_t m_count;

public:
  CalcMassCenter() : m_center(0.0, 0.0), m_count(0) {}

  void operator() (P const & p1, P const & p2, P const & p3)
  {
    ++m_count;
    m_center += p1;
    m_center += p2;
    m_center += p3;
  }
  P GetCenter() const { return m_center / (3*m_count); }
};

m2::PointD GetCenter(FeatureType const & f)
{
  feature::EGeomType const type = f.GetFeatureType();
  switch (type)
  {
  case feature::GEOM_POINT:
    return f.GetCenter();

  case feature::GEOM_LINE:
    {
      CalcPolyCenter doCalc;
      f.ForEachPointRef(doCalc, FeatureType::BEST_GEOMETRY);
      return doCalc.GetCenter();
    }

  default:
    {
      ASSERT_EQUAL ( type, feature::GEOM_AREA, () );
      CalcMassCenter doCalc;
      f.ForEachTriangleRef(doCalc, FeatureType::BEST_GEOMETRY);
      return doCalc.GetCenter();
    }
  }
}

}
