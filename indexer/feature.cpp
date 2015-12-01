#include "indexer/feature.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/feature_loader_base.hpp"
#include "indexer/classificator.hpp"

#include "geometry/distance.hpp"
#include "geometry/robust_orientation.hpp"

#include "platform/preferred_languages.hpp"

#include "defines.hpp" // just for file extensions


using namespace feature;

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureBase implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

void FeatureBase::Deserialize(feature::LoaderBase * pLoader, TBuffer buffer)
{
  m_pLoader = pLoader;
  m_pLoader->Init(buffer);

  m_limitRect = m2::RectD::GetEmptyRect();
  m_bTypesParsed = m_bCommonParsed = false;
  m_header = m_pLoader->GetHeader();
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
  switch (Header() & HEADER_GEOTYPE_MASK)
  {
  case HEADER_GEOM_LINE: return GEOM_LINE;
  case HEADER_GEOM_AREA: return GEOM_AREA;
  default: return GEOM_POINT;
  }
}

string FeatureBase::DebugString() const
{
  ASSERT(m_bCommonParsed, ());

  Classificator const & c = classif();

  string res = "Types";
  for (size_t i = 0; i < GetTypesCount(); ++i)
    res += (" : " + c.GetReadableObjectName(m_types[i]));
  res += "\n";

  return (res + m_params.DebugString());
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureType implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

void FeatureType::Deserialize(feature::LoaderBase * pLoader, TBuffer buffer)
{
  base_type::Deserialize(pLoader, buffer);

  m_pLoader->InitFeature(this);

  m_bHeader2Parsed = m_bPointsParsed = m_bTrianglesParsed = m_bMetadataParsed = false;

  m_innerStats.MakeZero();
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

void FeatureType::ResetGeometry() const
{
  m_points.clear();
  m_triangles.clear();

  if (GetFeatureType() != GEOM_POINT)
    m_limitRect = m2::RectD();

  m_bHeader2Parsed = m_bPointsParsed = m_bTrianglesParsed = false;

  m_pLoader->ResetGeometry();
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

void FeatureType::ParseMetadata() const
{
  if (m_bMetadataParsed) return;

  m_pLoader->ParseMetadata();

  if (HasInternet())
    m_metadata.Set(Metadata::FMD_INTERNET, "wlan");

  m_bMetadataParsed = true;
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

  switch (GetFeatureType())
  {
  case GEOM_POINT:
    s += (" Center:" + DebugPrint(m_center));
    break;

  case GEOM_LINE:
    s += " Points:";
    Points2String(s, m_points);
    break;

  case GEOM_AREA:
    s += " Triangles:";
    Points2String(s, m_triangles);
    break;

  case GEOM_UNDEFINED:
    ASSERT(false, ("Assume that we have valid feature always"));
    break;
  }

  return s;
}

string DebugPrint(FeatureType const & ft)
{
  return ft.DebugString(FeatureType::BEST_GEOMETRY);
}

bool FeatureType::IsEmptyGeometry(int scale) const
{
  ParseAll(scale);

  switch (GetFeatureType())
  {
  case GEOM_AREA: return m_triangles.empty();
  case GEOM_LINE: return m_points.empty();
  default: return false;
  }
}

m2::RectD FeatureType::GetLimitRect(int scale) const
{
  ParseAll(scale);

  if (m_triangles.empty() && m_points.empty() && (GetFeatureType() != GEOM_POINT))
  {
    // This function is called during indexing, when we need
    // to check visibility according to feature sizes.
    // So, if no geometry for this scale, assume that rect has zero dimensions.
    m_limitRect = m2::RectD(0, 0, 0, 0);
  }

  return m_limitRect;
}

void FeatureType::ParseAll(int scale) const
{
  ParseGeometry(scale);
  ParseTriangles(scale);
}

FeatureType::geom_stat_t FeatureType::GetGeometrySize(int scale) const
{
  uint32_t sz = ParseGeometry(scale);
  if (sz == 0 && !m_points.empty())
    sz = m_innerStats.m_points;

  return geom_stat_t(sz, m_points.size());
}

FeatureType::geom_stat_t FeatureType::GetTrianglesSize(int scale) const
{
  uint32_t sz = ParseTriangles(scale);
  if (sz == 0 && !m_triangles.empty())
    sz = m_innerStats.m_strips;

  return geom_stat_t(sz, m_triangles.size());
}

struct BestMatchedLangNames
{
  string m_defaultName;
  string m_nativeName;
  string m_intName;
  string m_englishName;

  bool operator()(int8_t code, string const & name)
  {
    static int8_t const defaultCode = StringUtf8Multilang::GetLangIndex("default");
    static int8_t const nativeCode = StringUtf8Multilang::GetLangIndex(languages::GetCurrentNorm());
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
  return m_params.house.Get();
}

bool FeatureType::GetName(int8_t lang, string & name) const
{
  if (!HasName())
    return false;

  ParseCommon();
  return m_params.name.GetString(lang, name);
}

uint8_t FeatureType::GetRank() const
{
  ParseCommon();
  return m_params.rank;
}

uint32_t FeatureType::GetPopulation() const
{
  uint8_t const r = GetRank();
  return (r == 0 ? 1 : static_cast<uint32_t>(pow(1.1, r)));
}

string FeatureType::GetRoadNumber() const
{
  ParseCommon();
  return m_params.ref;
}

bool FeatureType::HasInternet() const
{
  ParseTypes();

  bool res = false;

  ForEachType([&res](uint32_t type)
  {
    if (!res)
    {
      static const uint32_t t1 = classif().GetTypeByPath({"internet_access"});

      ftype::TruncValue(type, 1);
      res = (type == t1);
    }
  });

  return res;
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

    void operator() (m2::PointD const & pt)
    {
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

void FeatureType::SwapGeometry(FeatureType & r)
{
  ASSERT_EQUAL(m_bPointsParsed, r.m_bPointsParsed, ());
  ASSERT_EQUAL(m_bTrianglesParsed, r.m_bTrianglesParsed, ());

  if (m_bPointsParsed)
    m_points.swap(r.m_points);

  if (m_bTrianglesParsed)
    m_triangles.swap(r.m_triangles);
}
