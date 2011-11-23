#include "feature.hpp"
#include "feature_visibility.hpp"
#include "feature_loader_base.hpp"

#include "../platform/preferred_languages.hpp"

#include "../defines.hpp" // just for file extensions


using namespace feature;

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureBase implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

void FeatureBase::Deserialize(feature::LoaderBase * pLoader, BufferT buffer)
{
  m_pLoader.reset(pLoader);
  m_pLoader->Deserialize(buffer);

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

  m_pLoader->AssignFeature(this);

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
  string & m_defaultName;
  string m_nativeName;
  string m_intName;
  string m_englishName;
  BestMatchedLangNames(string & defaultName) : m_defaultName(defaultName) {}
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
      m_intName = name;
    else if (code == englishCode)
      m_englishName = name;
    return true;
  }
};

void FeatureType::GetPreferredDrawableNames(string & defaultName, string & intName) const
{
  ParseCommon();

  if (GetFeatureType() == GEOM_AREA)
    defaultName = m_Params.house.Get();

  if (defaultName.empty())
  {
    BestMatchedLangNames matcher(defaultName);
    ForEachNameRef(matcher);

    // match intName
    if (!matcher.m_nativeName.empty())
      intName.swap(matcher.m_nativeName);
    else if (!matcher.m_intName.empty())
      intName.swap(matcher.m_intName);
    else
      intName.swap(matcher.m_englishName);

    if (defaultName.empty())
      defaultName.swap(intName);
    else
    {  // filter out similar intName
      if (!intName.empty() && defaultName.find(intName) != string::npos)
        intName.clear();
    }
  }
  else
  {
    BestMatchedLangNames matcher(intName);
    ForEachNameRef(matcher);
  }
}

uint32_t FeatureType::GetPopulation() const
{
  ParseCommon();

  if (m_Params.rank == 0)
    return 1;

  return static_cast<uint32_t>(min(double(uint32_t(-1)), pow(1.1, m_Params.rank)));
}

double FeatureType::GetPopulationDrawRank() const
{
  uint32_t const n = GetPopulation();
  if (n == 1) return 0.0;

  // Do not return rank for countries.
  if (feature::IsCountry(m_Types, m_Types + GetTypesCount()))
    return 0.0;
  else
  {
    double const upperBound = 3.0E6;
    return min(upperBound, static_cast<double>(n)) / upperBound;
  }
}
