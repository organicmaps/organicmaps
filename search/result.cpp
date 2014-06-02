#include "result.hpp"
#include "geometry_utils.hpp"


namespace search
{

Result::Result(FeatureID const & id, m2::PointD const & fCenter,
               string const & str, string const & region,
               string const & flag, string const & type,
               uint32_t featureType, double distance)
  : m_id(id), m_center(fCenter), m_str(str), m_region(region),
    m_flag(flag), m_type(type), m_featureType(featureType), m_distance(distance)
{
  // Features with empty names can be found after suggestion.
  if (m_str.empty())
  {
    //m_str = "-";
    m_str = type;
  }
}

Result::Result(m2::PointD const & fCenter,
               string const & str, string const & region,
               string const & flag, double distance)
  : m_center(fCenter), m_str(str), m_region(region),
    m_flag(flag), m_featureType(0), m_distance(distance)
{
}

Result::Result(string const & str, string const & suggest)
  : m_str(str), m_suggestionStr(suggest)
{
}

Result::ResultType Result::GetResultType() const
{
  if (!m_suggestionStr.empty())
    return RESULT_SUGGESTION;
  if (m_id.IsValid())
    return RESULT_FEATURE;
  else
    return RESULT_LATLON;
}

bool Result::IsSuggest() const
{
  return !m_suggestionStr.empty();
}

FeatureID Result::GetFeatureID() const
{
  ASSERT_EQUAL(GetResultType(), RESULT_FEATURE, ());
  return m_id;
}

double Result::GetDistance() const
{
  ASSERT(!IsSuggest(), ());
  return m_distance;
}

m2::PointD Result::GetFeatureCenter() const
{
  ASSERT(!IsSuggest(), ());
  return m_center;
}

char const * Result::GetSuggestionString() const
{
  ASSERT(IsSuggest(), ());
  return m_suggestionStr.c_str();
}

bool Result::operator== (Result const & r) const
{
  ResultType const type = GetResultType();
  if (type == r.GetResultType())
  {
    if (type == RESULT_SUGGESTION)
      return m_suggestionStr == r.m_suggestionStr;
    else
    {
      // This function is used to filter duplicate results in cases:
      // - emitted World.mwm and Country.mwm
      // - after additional search in all mwm
      // so it's suitable here to test for 500m
      return (m_str == r.m_str && m_region == r.m_region &&
              m_featureType == r.m_featureType &&
              PointDistance(m_center, r.m_center) < 500.0);
    }
  }
  return false;
}

bool Results::AddResultCheckExisting(Result const & r)
{
  if (find(m_vec.begin(), m_vec.end(), r) == m_vec.end())
  {
    AddResult(r);
    return true;
  }
  else
    return false;
}

////////////////////////////////////////////////////////////////////////////////////
// AddressInfo implementation
////////////////////////////////////////////////////////////////////////////////////

void AddressInfo::MakeFrom(Result const & res)
{
  ASSERT_NOT_EQUAL(res.GetResultType(), Result::RESULT_SUGGESTION, ());

  // push the feature type (may be empty for coordinates result)
  string const type = res.GetFeatureType();
  if (!type.empty())
    m_types.push_back(type);

  // assign name if it's not equal with type
  string name = res.GetString();
  if (name != type)
      m_name.swap(name);
}

void AddressInfo::SetPinName(string const & name)
{
  m_name = name;
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
