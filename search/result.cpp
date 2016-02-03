#include "result.hpp"
#include "geometry_utils.hpp"
#include "search_common.hpp"

namespace search
{
Result::Result(FeatureID const & id, m2::PointD const & pt, string const & str,
               string const & region, string const & type, uint32_t featureType,
               Metadata const & meta)
  : m_id(id)
  , m_center(pt)
  , m_str(str)
  , m_region(region)
  , m_type(type)
  , m_featureType(featureType)
  , m_positionInResults(-1)
  , m_metadata(meta)
{
  Init(true /* metadataInitialized */);
}

Result::Result(FeatureID const & id, m2::PointD const & pt, string const & str, string const & type)
  : m_id(id), m_center(pt), m_str(str), m_type(type), m_positionInResults(-1)
{
  Init(false /* metadataInitialized */);
}

Result::Result(m2::PointD const & pt, string const & str, string const & region,
               string const & type)
  : m_center(pt), m_str(str), m_region(region), m_type(type), m_positionInResults(-1)
{
}

Result::Result(string const & str, string const & suggest)
  : m_str(str), m_suggestionStr(suggest), m_positionInResults(-1)
{
}

Result::Result(Result const & res, string const & suggest)
  : m_id(res.m_id)
  , m_center(res.m_center)
  , m_str(res.m_str)
  , m_region(res.m_region)
  , m_type(res.m_type)
  , m_featureType(res.m_featureType)
  , m_suggestionStr(suggest)
  , m_hightlightRanges(res.m_hightlightRanges)
  , m_positionInResults(-1)
{
}

void Result::Init(bool metadataInitialized)
{
  // Features with empty names can be found after suggestion.
  if (m_str.empty())
    m_str = m_type;

  m_metadata.m_isInitialized = metadataInitialized;
}

Result::ResultType Result::GetResultType() const
{
  bool const idValid = m_id.IsValid();

  if (!m_suggestionStr.empty())
    return (idValid ? RESULT_SUGGEST_FROM_FEATURE : RESULT_SUGGEST_PURE);

  if (idValid)
    return RESULT_FEATURE;
  else
    return (m_type.empty() ? RESULT_LATLON : RESULT_ADDRESS);
}

bool Result::IsSuggest() const
{
  return !m_suggestionStr.empty();
}

bool Result::HasPoint() const
{
  return (GetResultType() != RESULT_SUGGEST_PURE);
}

FeatureID Result::GetFeatureID() const
{
  ASSERT_EQUAL(GetResultType(), RESULT_FEATURE, ());
  return m_id;
}

m2::PointD Result::GetFeatureCenter() const
{
  ASSERT(HasPoint(), ());
  return m_center;
}

char const * Result::GetSuggestionString() const
{
  ASSERT(IsSuggest(), ());
  return m_suggestionStr.c_str();
}

bool Result::IsEqualSuggest(Result const & r) const
{
  return (m_suggestionStr == r.m_suggestionStr);
}

bool Result::IsEqualFeature(Result const & r) const
{
  ResultType const type = GetResultType();
  if (type != r.GetResultType())
    return false;

  if (type == RESULT_ADDRESS)
    return (PointDistance(m_center, r.m_center) < 50.0);
  else
  {
    ASSERT_EQUAL(type, Result::RESULT_FEATURE, ());

    ASSERT(m_id.IsValid() && r.m_id.IsValid(), ());
    if (m_id == r.m_id)
      return true;

    // This function is used to filter duplicate results in cases:
    // - emitted World.mwm and Country.mwm
    // - after additional search in all mwm
    // so it's suitable here to test for 500m
    return (m_str == r.m_str && m_region == r.m_region &&
            m_featureType == r.m_featureType &&
            PointDistance(m_center, r.m_center) < 500.0);
  }
}

void Result::AddHighlightRange(pair<uint16_t, uint16_t> const & range)
{
  m_hightlightRanges.push_back(range);
}

pair<uint16_t, uint16_t> const & Result::GetHighlightRange(size_t idx) const
{
  ASSERT(idx < m_hightlightRanges.size(), ());
  return m_hightlightRanges[idx];
}

void Result::AppendCity(string const & name)
{
  if (name.empty())
    return;

  if (m_region.empty())
    m_region = name;
  else
    m_region += (", " + name);
}

string Result::ToStringForStats() const
{
  string s;
  s.append(GetString());
  s.append("|");
  s.append(GetFeatureType());
  s.append("|");
  s.append(IsSuggest() ? "1" : "0");
  return s;
}

bool Results::AddResult(Result && res)
{
  // Find first feature result.
  auto it = find_if(m_vec.begin(), m_vec.end(), [](Result const & r)
  {
    switch (r.GetResultType())
    {
    case Result::RESULT_FEATURE:
    case Result::RESULT_ADDRESS:
      return true;
    default:
      return false;
    }
  });

  if (res.IsSuggest())
  {
    if (distance(m_vec.begin(), it) >= MAX_SUGGESTS_COUNT)
      return false;

    for (auto i = m_vec.begin(); i != it; ++i)
      if (res.IsEqualSuggest(*i))
        return false;

    for (auto i = it; i != m_vec.end(); ++i)
    {
      auto & r = *i;
      auto const oldPos = r.GetPositionInResults();
      r.SetPositionInResults(oldPos + 1);
    }
    res.SetPositionInResults(distance(m_vec.begin(), it));
    m_vec.insert(it, move(res));
  }
  else
  {
    for (; it != m_vec.end(); ++it)
      if (res.IsEqualFeature(*it))
        return false;

    res.SetPositionInResults(m_vec.size());
    m_vec.push_back(move(res));
  }

  return true;
}

size_t Results::GetSuggestsCount() const
{
  size_t res = 0;
  for (size_t i = 0; i < GetCount(); i++)
  {
    if (m_vec[i].IsSuggest())
      ++res;
    else
    {
      // Suggests always go first, so we can exit here.
      break;
    }
  }
  return res;
}

////////////////////////////////////////////////////////////////////////////////////
// AddressInfo implementation
////////////////////////////////////////////////////////////////////////////////////

void AddressInfo::MakeFrom(Result const & res)
{
  ASSERT_NOT_EQUAL(res.GetResultType(), Result::RESULT_SUGGEST_PURE, ());

  string const & type = res.GetFeatureType();
  if (!type.empty())
    m_types.push_back(type);

  // assign name if it's not equal with type
  string const & name = res.GetString();
  if (name != type)
    m_name = name;
}

bool AddressInfo::IsEmptyName() const
{
  return m_name.empty() && m_house.empty();
}

string AddressInfo::GetPinName() const
{
  if (IsEmptyName() && !m_types.empty())
    return m_types[0];
  else
    return m_name.empty() ? m_house : m_name;
}

string AddressInfo::GetPinType() const
{
  return GetBestType();
}

string AddressInfo::FormatPinText() const
{
  // select name or house if name is empty
  string const & ret = (m_name.empty() ? m_house : m_name);

  string const type = GetBestType();
  if (type.empty())
    return ret;

  return (ret.empty() ? type : (ret + " (" + type + ')'));
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

string AddressInfo::GetBestType() const
{
  if (m_types.empty())
    return string();

  /// @todo Probably, we should skip some "common" types here like in TypesHolder::SortBySpec.
  ASSERT(!m_types[0].empty(), ());
  return m_types[0];
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
