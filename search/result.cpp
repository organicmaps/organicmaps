#include "search/result.hpp"

#include "search/common.hpp"
#include "search/geometry_utils.hpp"

namespace search
{
// Result ------------------------------------------------------------------------------------------
Result::Result(FeatureID const & id, m2::PointD const & pt, string const & str,
               string const & address, string const & type, uint32_t featureType,
               Metadata const & meta)
  : m_id(id)
  , m_center(pt)
  , m_str(str.empty() ? type : str)  //!< Features with empty names can be found after suggestion.
  , m_address(address)
  , m_type(type)
  , m_featureType(featureType)
  , m_metadata(meta)
{
}

Result::Result(m2::PointD const & pt, string const & latlon, string const & address)
  : m_center(pt), m_str(latlon), m_address(address)
{
}

Result::Result(string const & str, string const & suggest)
  : m_str(str), m_suggestionStr(suggest)
{
}

Result::Result(Result const & res, string const & suggest)
  : m_id(res.m_id)
  , m_center(res.m_center)
  , m_str(res.m_str)
  , m_address(res.m_address)
  , m_type(res.m_type)
  , m_featureType(res.m_featureType)
  , m_suggestionStr(suggest)
  , m_hightlightRanges(res.m_hightlightRanges)
{
}

Result::ResultType Result::GetResultType() const
{
  bool const idValid = m_id.IsValid();

  if (!m_suggestionStr.empty())
    return (idValid ? RESULT_SUGGEST_FROM_FEATURE : RESULT_SUGGEST_PURE);

  if (idValid)
    return RESULT_FEATURE;
  else
    return RESULT_LATLON;
}

bool Result::IsSuggest() const
{
  return !m_suggestionStr.empty();
}

bool Result::HasPoint() const
{
  return (GetResultType() != RESULT_SUGGEST_PURE);
}

FeatureID const & Result::GetFeatureID() const
{
#if defined(DEBUG)
  auto const type = GetResultType();
  ASSERT(type == RESULT_FEATURE, (type));
#endif
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

  ASSERT_EQUAL(type, Result::RESULT_FEATURE, ());

  ASSERT(m_id.IsValid() && r.m_id.IsValid(), ());
  if (m_id == r.m_id)
    return true;

  // This function is used to filter duplicate results in cases:
  // - emitted World.mwm and Country.mwm
  // - after additional search in all mwm
  // so it's suitable here to test for 500m
  return (m_str == r.m_str && m_address == r.m_address &&
          m_featureType == r.m_featureType &&
          PointDistance(m_center, r.m_center) < 500.0);
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
  // Prepend only if city is absent in region (mwm) name.
  if (m_address.find(name) == string::npos)
    m_address = name + ", " + m_address;
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

// Results -----------------------------------------------------------------------------------------
Results::Results()
{
  Clear();
}

bool Results::AddResult(Result && result)
{
  // Find first feature result.
  auto it = find_if(m_results.begin(), m_results.end(), [](Result const & r)
  {
    switch (r.GetResultType())
    {
    case Result::RESULT_FEATURE:
      return true;
    default:
      return false;
    }
  });

  if (result.IsSuggest())
  {
    if (distance(m_results.begin(), it) >= kMaxNumSuggests)
      return false;

    for (auto i = m_results.begin(); i != it; ++i)
      if (result.IsEqualSuggest(*i))
        return false;
    InsertResult(it, move(result));
  }
  else
  {
    for (; it != m_results.end(); ++it)
    {
      if (result.IsEqualFeature(*it))
        return false;
    }
    InsertResult(m_results.end(), move(result));
  }

  return true;
}

void Results::AddResultNoChecks(Result && result)
{
  InsertResult(m_results.end(), move(result));
}

void Results::AddResultsNoChecks(ConstIter first, ConstIter last)
{
  while (first != last)
  {
    auto resultCopy = *first++;
    AddResultNoChecks(move(resultCopy));
  }
}

void Results::Clear()
{
  m_results.clear();
  m_status = Status::None;
}

size_t Results::GetSuggestsCount() const
{
  size_t res = 0;
  for (size_t i = 0; i < GetCount(); i++)
  {
    if (m_results[i].IsSuggest())
      ++res;
    else
    {
      // Suggests always go first, so we can exit here.
      break;
    }
  }
  return res;
}

void Results::InsertResult(vector<Result>::iterator where, Result && result)
{
  ASSERT_LESS(m_results.size(), numeric_limits<int32_t>::max(), ());

  for (auto it = where; it != m_results.end(); ++it)
  {
    auto & r = *it;
    auto const position = r.GetPositionInResults();
    r.SetPositionInResults(position + 1);
  }

  result.SetPositionInResults(static_cast<int32_t>(distance(m_results.begin(), where)));
  m_results.insert(where, move(result));
}

// AddressInfo -------------------------------------------------------------------------------------
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

string AddressInfo::FormatHouseAndStreet(AddressType type /* = DEFAULT */) const
{
  // Check whether we can format address according to the query type and actual address distance.
  /// @todo We can add "Near" prefix here in future according to the distance.
  if (m_distanceMeters > 0.0)
  {
    if (type == SEARCH_RESULT && m_distanceMeters > 50.0)
      return string();
    if (m_distanceMeters > 200.0)
      return string();
  }

  string result = m_street;
  if (!m_house.empty())
  {
    if (!result.empty())
      result += ", ";
    result += m_house;
  }

  return result;
}

string AddressInfo::FormatAddress(AddressType type /* = DEFAULT */) const
{
  string result = FormatHouseAndStreet(type);
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

string AddressInfo::FormatNameAndAddress(AddressType type /* = DEFAULT */) const
{
  string const addr = FormatAddress(type);
  return (m_name.empty() ? addr : m_name + ", " + addr);
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

string DebugPrint(AddressInfo const & info)
{
  return info.FormatNameAndAddress();
}

string DebugPrint(Result const & result)
{
  return "Result { Name: " + result.GetString() + "; Type: " + result.GetFeatureType() +
         "; Info: " + DebugPrint(result.GetRankingInfo()) + " }";
}
}  // namespace search
