#include "feature.hpp"
#include "feature_visibility.hpp"
#include "feature_loader_base.hpp"

#include "../geometry/distance.hpp"
#include "../geometry/robust_orientation.hpp"

#include "../platform/preferred_languages.hpp"

#include "../defines.hpp" // just for file extensions


using namespace feature;

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureBase implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

void FeatureBase::Deserialize(feature::LoaderBase * pLoader, BufferT buffer)
{
  m_pLoader = pLoader;
  m_pLoader->Init(buffer);

  m_LimitRect = m2::RectD::GetEmptyRect();
  m_bTypesParsed = m_bCommonParsed = false;
  m_Header = m_pLoader->GetHeader();
}

void FeatureBase::ParseTypes() const
{
  if (!m_bTypesParsed)
  {
    m_pLoader->ParseTypes();
    m_bTypesParsed = true;
  }
}

void FeatureBase::ParseCommon() const
{
  if (!m_bCommonParsed)
  {
    ParseTypes();

    m_pLoader->ParseCommon();
    m_bCommonParsed = true;
  }
}

feature::EGeomType FeatureBase::GetFeatureType() const
{
  uint8_t const h = (Header() & HEADER_GEOTYPE_MASK);

  if (h & HEADER_GEOM_AREA)
    return GEOM_AREA;
  else if (h & HEADER_GEOM_LINE)
    return GEOM_LINE;
  else
  {
    ASSERT ( h == HEADER_GEOM_POINT, (h) );
    return GEOM_POINT;
  }
}

string FeatureBase::DebugString() const
{
  ASSERT(m_bCommonParsed, ());

  string res("FEATURE: ");

  for (size_t i = 0; i < GetTypesCount(); ++i)
    res += "Type:" + DebugPrint(m_Types[i]) + " ";

  res += m_Params.DebugString();

  if (GetFeatureType() == GEOM_POINT)
    res += "Center:" + DebugPrint(m_Center) + " ";

  return res;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureType implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

void FeatureType::Deserialize(feature::LoaderBase * pLoader, BufferT buffer)
{
  base_type::Deserialize(pLoader, buffer);

  m_pLoader->InitFeature(this);

  m_bHeader2Parsed = m_bPointsParsed = m_bTrianglesParsed = false;

  m_InnerStats.MakeZero();
}

void FeatureType::ParseHeader2() const
{
  if (!m_bHeader2Parsed)
  {
    ParseCommon();

    m_pLoader->ParseHeader2();
    m_bHeader2Parsed = true;
  }
}

uint32_t FeatureType::ParseGeometry(int scale) const
{
  uint32_t sz = 0;
  if (!m_bPointsParsed)
  {
    ParseHeader2();

    sz = m_pLoader->ParseGeometry(scale);
    m_bPointsParsed = true;
  }
  return sz;
}

uint32_t FeatureType::ParseTriangles(int scale) const
{
  uint32_t sz = 0;
  if (!m_bTrianglesParsed)
  {
    ParseHeader2();

    sz = m_pLoader->ParseTriangles(scale);
    m_bTrianglesParsed = true;
  }
  return sz;
}

namespace
{
  template <class TCont>
  void Points2String(string & s, TCont const & points)
  {
    for (size_t i = 0; i < points.size(); ++i)
      s += DebugPrint(points[i]) + " ";
  }
}

string FeatureType::DebugString(int scale) const
{
  ParseAll(scale);

  string s = base_type::DebugString();

  s += "Points:";
  Points2String(s, m_Points);

  s += "Triangles:";
  Points2String(s, m_Triangles);

  return s;
}

bool FeatureType::IsEmptyGeometry(int scale) const
{
  ParseAll(scale);

  switch (GetFeatureType())
  {
  case GEOM_AREA: return m_Triangles.empty();
  case GEOM_LINE: return m_Points.empty();
  default: return false;
  }
}

m2::RectD FeatureType::GetLimitRect(int scale) const
{
  ParseAll(scale);

  if (m_Triangles.empty() && m_Points.empty() && (GetFeatureType() != GEOM_POINT))
  {
    // This function is called during indexing, when we need
    // to check visibility according to feature sizes.
    // So, if no geometry for this scale, assume that rect has zero dimensions.
    m_LimitRect = m2::RectD(0, 0, 0, 0);
  }

  return m_LimitRect;
}

void FeatureType::ParseAll(int scale) const
{
  ParseGeometry(scale);
  ParseTriangles(scale);
}

FeatureType::geom_stat_t FeatureType::GetGeometrySize(int scale) const
{
  uint32_t sz = ParseGeometry(scale);
  if (sz == 0 && !m_Points.empty())
    sz = m_InnerStats.m_Points;

  return geom_stat_t(sz, m_Points.size());
}

FeatureType::geom_stat_t FeatureType::GetTrianglesSize(int scale) const
{
  uint32_t sz = ParseTriangles(scale);
  if (sz == 0 && !m_Triangles.empty())
    sz = m_InnerStats.m_Strips;

  return geom_stat_t(sz, m_Triangles.size());
}

struct BestMatchedLangNames
{
  string m_defaultName;
  string m_nativeName;
  string m_intName;
  string m_englishName;

  bool operator()(int8_t code, string const & name)
  {
    static int8_t defaultCode = StringUtf8Multilang::GetLangIndex("default");
    // @TODO support list of preferred languages
    // We can get them also from input keyboard languages
    static int8_t const nativeCode = StringUtf8Multilang::GetLangIndex(languages::CurrentLanguage());
    static int8_t const intCode = StringUtf8Multilang::GetLangIndex("int_name");
    static int8_t const englishCode = StringUtf8Multilang::GetLangIndex("en");

    if (code == defaultCode)
      m_defaultName = name;
    else if (code == nativeCode)
      m_nativeName = name;
    else if (code == intCode)
    {
      // There are many "junk" names in Arabian island.
      m_intName = name.substr(0, name.find_first_of(','));
      // int_name should be used as name:en when name:en not found
      if ((nativeCode == englishCode) && m_nativeName.empty())
        m_nativeName = m_intName;
    }
    else if (code == englishCode)
      m_englishName = name;
    return true;
  }
};

void FeatureType::GetPreferredNames(string & defaultName, string & intName) const
{
  ParseCommon();

  BestMatchedLangNames matcher;
  ForEachNameRef(matcher);

  defaultName.swap(matcher.m_defaultName);

  if (!matcher.m_nativeName.empty())
    intName.swap(matcher.m_nativeName);
  else if (!matcher.m_intName.empty())
    intName.swap(matcher.m_intName);
  else
    intName.swap(matcher.m_englishName);

  if (defaultName.empty())
    defaultName.swap(intName);
  else
  {
    // filter out similar intName
    if (!intName.empty() && defaultName.find(intName) != string::npos)
      intName.clear();
  }
}

void FeatureType::GetReadableName(string & name) const
{
  ParseCommon();

  BestMatchedLangNames matcher;
  ForEachNameRef(matcher);

  if (!matcher.m_nativeName.empty())
    name.swap(matcher.m_nativeName);
  else if (!matcher.m_defaultName.empty())
    name.swap(matcher.m_defaultName);
  else if (!matcher.m_intName.empty())
    name.swap(matcher.m_intName);
  else
    name.swap(matcher.m_englishName);
}

string FeatureType::GetHouseNumber() const
{
  ParseCommon();
  return (GetFeatureType() == GEOM_AREA ? m_Params.house.Get() : string());
}

bool FeatureType::GetName(int8_t lang, string & name) const
{
  if (!HasName())
    return false;

  ParseCommon();
  return m_Params.name.GetString(lang, name);
}

uint8_t FeatureType::GetRank() const
{
  ParseCommon();
  return m_Params.rank;
}

uint32_t FeatureType::GetPopulation() const
{
  uint8_t const r = GetRank();
  return (r == 0 ? 1 : static_cast<uint32_t>(pow(1.1, r)));
}

string FeatureType::GetRoadNumber() const
{
  ParseCommon();
  return m_Params.ref;
}

namespace
{
  class DoCalcDistance
  {
    m2::PointD m_prev, m_pt;
    bool m_hasPrev;

    static double Inf() { return numeric_limits<double>::max(); }

    static double GetDistance(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p)
    {
      m2::DistanceToLineSquare<m2::PointD> calc;
      calc.SetBounds(p1, p2);
      return sqrt(calc(p));
    }

  public:
    DoCalcDistance(m2::PointD const & pt)
      : m_pt(pt), m_hasPrev(false), m_dist(Inf())
    {
    }

    void TestPoint(m2::PointD const & p)
    {
      m_dist = m_pt.Length(p);
    }

    void operator() (CoordPointT const & p)
    {
      m2::PointD pt(p.first, p.second);

      if (m_hasPrev)
        m_dist = min(m_dist, GetDistance(m_prev, pt, m_pt));
      else
        m_hasPrev = true;

      m_prev = pt;
    }

    void operator() (m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
    {
      m2::PointD arrP[] = { p1, p2, p3 };

      // make right-oriented triangle
      if (m2::robust::OrientedS(arrP[0], arrP[1], arrP[2]) < 0.0)
        swap(arrP[1], arrP[2]);

      double d = Inf();
      for (size_t i = 0; i < 3; ++i)
      {
        double const s = m2::robust::OrientedS(arrP[i], arrP[(i + 1) % 3], m_pt);
        if (s < 0.0)
          d = min(d, GetDistance(arrP[i], arrP[(i + 1) % 3], m_pt));
      }

      m_dist = ((d == Inf()) ? 0.0 : min(m_dist, d));
    }

    double m_dist;
  };
}

double FeatureType::GetDistance(m2::PointD const & pt, int scale) const
{
  DoCalcDistance calc(pt);

  switch (GetFeatureType())
  {
  case GEOM_POINT: calc.TestPoint(GetCenter()); break;
  case GEOM_LINE: ForEachPointRef(calc, scale); break;
  case GEOM_AREA: ForEachTriangleRef(calc, scale); break;
  default:
    CHECK ( false, () );
  }

  return calc.m_dist;
}
