#include "../base/SRC_FIRST.hpp"

#include "feature.hpp"
#include "feature_visibility.hpp"
#include "feature_loader_base.hpp"

#include "../defines.hpp" // just for file extensions

#include "../base/start_mem_debug.hpp"


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
    res += "Type:" + debug_print(m_Types[i]) + " ";

  res += m_Params.DebugString();

  if (GetFeatureType() == GEOM_POINT)
    res += "Center:" + debug_print(m_Center) + " ";

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
      s += debug_print(points[i]) + " ";
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

class BestMatchedLangName
{
  int8_t const * m_priorities;
  string & m_result;
  int m_minPriority;

public:
  BestMatchedLangName(int8_t const * priorities, string & result)
    : m_priorities(priorities), m_result(result), m_minPriority(256)
  {
  }

  bool operator() (int8_t lang, string const & utf8s)
  {
    ASSERT(lang >= 0 && lang < MAX_SUPPORTED_LANGUAGES, ());
    int8_t const priority = m_priorities[lang];
    if (priority == 0)
    {
      m_result = utf8s;
      return false; // stop foreach
    }
    if (priority < m_minPriority)
    {
      m_minPriority = priority;
      m_result = utf8s;
    }
    return true;
  }
};

string FeatureType::GetPreferredDrawableName(int8_t const * priorities) const
{
  ParseCommon();

  string res;
  if (priorities)
  {
    BestMatchedLangName matcher(priorities, res);
    ForEachNameRef(matcher);
  }
  else
    m_Params.name.GetString(0, res);

  if (res.empty() && GetFeatureType() == GEOM_AREA)
    res = m_Params.house.Get();

  return res;
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

uint8_t FeatureType::GetSearchRank() const
{
  ParseCommon();
  return m_Params.rank;
}
