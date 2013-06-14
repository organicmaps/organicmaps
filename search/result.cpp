#include "result.hpp"


namespace search
{

Result::Result(string const & str, string const & region,
               string const & flag, string const & type,
               uint32_t featureType, m2::RectD const & featureRect,
               double distanceFromCenter)
  : m_str(str), m_region(region), m_flag(flag), m_type(type),
    m_featureRect(featureRect), m_featureType(featureType),
    m_distanceFromCenter(distanceFromCenter)
{
  // Features with empty names can be found after suggestion.
  if (m_str.empty())
  {
    //m_str = "-";
    m_str = type;
  }
}

Result::Result(string const & str, string const & suggestionStr)
  : m_str(str), m_suggestionStr(suggestionStr)
{
}

Result::ResultType Result::GetResultType() const
{
  if (!m_suggestionStr.empty())
    return RESULT_SUGGESTION;
  return RESULT_FEATURE;
}

m2::RectD Result::GetFeatureRect() const
{
  ASSERT_EQUAL(GetResultType(), RESULT_FEATURE, ());
  return m_featureRect;
}

m2::PointD Result::GetFeatureCenter() const
{
  ASSERT_EQUAL(GetResultType(), RESULT_FEATURE, ());
  return m_featureRect.Center();
}

double Result::GetDistanceFromCenter() const
{
  ASSERT_EQUAL(GetResultType(), RESULT_FEATURE, ());
  return m_distanceFromCenter;
}

char const * Result::GetSuggestionString() const
{
  ASSERT_EQUAL(GetResultType(), RESULT_SUGGESTION, ());
  return m_suggestionStr.c_str();
}

bool Result::operator== (Result const & r) const
{
  return (m_str == r.m_str && m_region == r.m_region && m_featureType == r.m_featureType &&
          GetResultType() == r.GetResultType() &&
          my::AlmostEqual(m_distanceFromCenter, r.m_distanceFromCenter));
}

void Results::AddResultCheckExisting(Result const & r)
{
  if (find(m_vec.begin(), m_vec.end(), r) == m_vec.end())
    AddResult(r);
}

////////////////////////////////////////////////////////////////////////////////////
// AddressInfo implementation
////////////////////////////////////////////////////////////////////////////////////

void AddressInfo::MakeFrom(Result const & res)
{
  ASSERT_EQUAL ( res.GetResultType(), Result::RESULT_FEATURE, () );

  // push the feature type
  string const type = res.GetFeatureType();
  if (!type.empty())
    m_types.push_back(type);
  else
  {
    ASSERT ( false, ("Search result with empty type") );
  }

  // assign name if it's not equal with type
  string name = res.GetString();
  if (name != type)
    m_name.swap(name);
}

string AddressInfo::GetPinName() const
{
  return m_name.empty() ? m_house : m_name;
}

string AddressInfo::GetPinType() const
{
  char const * type = GetBestType();
  return (type ? type : "");
}

string AddressInfo::FormatPinText() const
{
  // select name or house if name is empty
  string const & ret = (m_name.empty() ? m_house : m_name);

  char const * type = GetBestType();
  if (type)
  {
    if (ret.empty())
      return type;
    else
      return ret + " (" + string(type) + ')';
  }
  else
    return ret;
}

string AddressInfo::FormatAddress() const
{
  string result = m_house;
  if (!m_street.empty())
  {
    if (!result.empty())
      result += ' ';
    result += m_street;
  }
  if (!m_city.empty())
  {
    if (!result.empty())
      result += ", ";
    result += m_city;
  }
  if (!m_country.empty())
  {
    if (!result.empty())
      result += ", ";
    result += m_country;
  }
  return result;
}

string AddressInfo::FormatTypes() const
{
  string result;
  for (size_t i = 0; i < m_types.size(); ++i)
  {
    ASSERT ( !m_types.empty(), () );
    if (!result.empty())
      result += ' ';
    result += m_types[i];
  }
  return result;
}

string AddressInfo::FormatNameAndAddress() const
{
  string const addr = FormatAddress();
  return (m_name.empty() ? addr : m_name + ", " + addr);
}

char const * AddressInfo::GetBestType() const
{
  if (!m_types.empty())
  {
    ASSERT ( !m_types[0].empty(), () );
    return m_types[0].c_str();
  }
  return 0;
}

void AddressInfo::Clear()
{
  m_country.clear();
  m_city.clear();
  m_street.clear();
  m_house.clear();
  m_name.clear();
  m_types.clear();
}

}  // namespace search
