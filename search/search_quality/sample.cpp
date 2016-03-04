#include "sample.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/sstream.hpp"
#include "std/string.hpp"

#include "3party/jansson/myjansson.hpp"

namespace
{
void FromJSON(json_t * root, string & result)
{
  result = string(json_string_value(root));
}

void FromJSONObject(json_t * root, string const & field, string & result)
{
  if (!json_is_object(root))
    MYTHROW(my::Json::Exception, ("Bad json object when parsing", field));
  json_t * val = json_object_get(root, field.c_str());
  if (!val)
    MYTHROW(my::Json::Exception, ("Obligatory field", field, "is absent."));
  if (!json_is_string(val))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json string."));
  result = string(json_string_value(val));
}

void FromJSONObject(json_t * root, string const & field, strings::UniString & result)
{
  string s;
  FromJSONObject(root, field, s);
  result = strings::MakeUniString(s);
}

void FromJSONObject(json_t * root, string const & field, double & result)
{
  if (!json_is_object(root))
    MYTHROW(my::Json::Exception, ("Bad json object when parsing", field));
  json_t * val = json_object_get(root, field.c_str());
  if (!val)
    MYTHROW(my::Json::Exception, ("Obligatory field", field, "is absent."));
  if (!json_is_number(val))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json number."));
  result = json_number_value(val);
}

void FromJSONObject(json_t * root, string const & field, m2::PointD & result)
{
  if (!json_is_object(root))
    MYTHROW(my::Json::Exception, ("Bad json object when parsing", field));
  json_t * val = json_object_get(root, field.c_str());
  if (!val)
    MYTHROW(my::Json::Exception, ("Obligatory field", field, "is absent."));
  FromJSONObject(val, "x", result.x);
  FromJSONObject(val, "y", result.y);
}

void FromJSONObject(json_t * root, string const & field, m2::RectD & result)
{
  json_t * rect = json_object_get(root, field.c_str());
  if (!rect)
    MYTHROW(my::Json::Exception, ("Obligatory field", field, "is absent."));
  double minX, minY, maxX, maxY;
  FromJSONObject(rect, "minx", minX);
  FromJSONObject(rect, "miny", minY);
  FromJSONObject(rect, "maxx", maxX);
  FromJSONObject(rect, "maxy", maxY);
  result.setMinX(minX);
  result.setMinY(minY);
  result.setMaxX(maxX);
  result.setMaxY(maxY);
}

void FromJSONObject(json_t * root, string const & field, search::Sample::Result::Relevance & r)
{
  string relevance;
  FromJSONObject(root, field, relevance);
  if (relevance == "vital")
    r = search::Sample::Result::Relevance::RELEVANCE_VITAL;
  else if (relevance == "relevant")
    r = search::Sample::Result::Relevance::RELEVANCE_RELEVANT;
  else
    r = search::Sample::Result::Relevance::RELEVANCE_IRRELEVANT;
}

void FromJSON(json_t * root, search::Sample::Result & result);

template <typename T>
void FromJSONObject(json_t * root, string const & field, vector<T> & result)
{
  json_t * arr = json_object_get(root, field.c_str());
  if (!arr)
    MYTHROW(my::Json::Exception, ("Obligatory field", field, "is absent."));
  if (!json_is_array(arr))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json array."));
  size_t sz = json_array_size(arr);
  result.resize(sz);
  for (size_t i = 0; i < sz; ++i)
    FromJSON(json_array_get(arr, i), result[i]);
}

void FromJSON(json_t * root, search::Sample::Result & result)
{
  FromJSONObject(root, "position", result.m_pos);
  FromJSONObject(root, "name", result.m_name);
  FromJSONObject(root, "houseNumber", result.m_houseNumber);
  FromJSONObject(root, "types", result.m_types);
  FromJSONObject(root, "relevancy", result.m_relevance);
}

bool LessRect(m2::RectD const & lhs, m2::RectD const & rhs)
{
  if (lhs.minX() != rhs.minX())
    return lhs.minX() < rhs.minX();
  if (lhs.minY() != rhs.minY())
    return lhs.minY() < rhs.minY();
  if (lhs.maxX() != rhs.maxX())
    return lhs.maxX() < rhs.maxX();
  if (lhs.maxY() != rhs.maxY())
    return lhs.maxY() < rhs.maxY();
  return false;
}

template <typename T>
bool Less(vector<T> lhs, vector<T> rhs)
{
  sort(lhs.begin(), lhs.end());
  sort(rhs.begin(), rhs.end());
  return lhs < rhs;
}

template <typename T>
bool Equal(vector<T> lhs, vector<T> rhs)
{
  sort(lhs.begin(), lhs.end());
  sort(rhs.begin(), rhs.end());
  return lhs == rhs;
}
}  // namespace

namespace search
{
bool Sample::Result::operator<(Sample::Result const & rhs) const
{
  if (m_pos != rhs.m_pos)
    return m_pos < rhs.m_pos;
  if (m_name != rhs.m_name)
    return m_name < rhs.m_name;
  if (m_houseNumber != rhs.m_houseNumber)
    return m_houseNumber < rhs.m_houseNumber;
  if (m_relevance != rhs.m_relevance)
    return m_relevance < rhs.m_relevance;
  return Less(m_types, rhs.m_types);
}

bool Sample::Result::operator==(Sample::Result const & rhs) const
{
  // Note: Strict equality for points and viewports.
  return m_pos == rhs.m_pos && m_name == rhs.m_name && m_houseNumber == rhs.m_houseNumber &&
         Equal(m_types, rhs.m_types) && m_relevance == rhs.m_relevance;
}

bool Sample::DeserializeFromJSON(string const & jsonStr)
{
  try
  {
    my::Json root(jsonStr.c_str());
    DeserializeFromJSONImpl(root.get());
    return true;
  }
  catch (my::Json::Exception const & e)
  {
    LOG(LDEBUG, ("Can't parse sample:", e.Msg(), jsonStr));
  }
  return false;
}

bool Sample::operator<(Sample const & rhs) const
{
  if (m_query != rhs.m_query)
    return m_query < rhs.m_query;
  if (m_locale != rhs.m_locale)
    return m_locale < rhs.m_locale;
  if (m_pos != rhs.m_pos)
    return m_pos < rhs.m_pos;
  if (m_viewport != rhs.m_viewport)
    return LessRect(m_viewport, rhs.m_viewport);
  return Less(m_results, rhs.m_results);
}

bool Sample::operator==(Sample const & rhs) const
{
  return m_query == rhs.m_query && m_locale == rhs.m_locale && m_pos == rhs.m_pos &&
         m_viewport == rhs.m_viewport && Equal(m_results, rhs.m_results);
}

// static
bool Sample::DeserializeFromJSON(string const & jsonStr, vector<Sample> & samples)
{
  try
  {
    my::Json root(jsonStr.c_str());
    if (!json_is_array(root.get()))
      MYTHROW(my::Json::Exception, ("The field", "samples", "must contain a json array."));
    size_t numSamples = json_array_size(root.get());
    samples.resize(numSamples);
    for (size_t i = 0; i < numSamples; ++i)
      samples[i].DeserializeFromJSONImpl(json_array_get(root.get(), i));
    return true;
  }
  catch (my::Json::Exception const & e)
  {
    LOG(LERROR, ("Can't parse samples:", e.Msg(), jsonStr));
  }
  return false;
}

void Sample::DeserializeFromJSONImpl(json_t * root)
{
  FromJSONObject(root, "query", m_query);
  FromJSONObject(root, "locale", m_locale);
  FromJSONObject(root, "position", m_pos);
  FromJSONObject(root, "viewport", m_viewport);
  FromJSONObject(root, "results", m_results);
}

string DebugPrint(Sample::Result::Relevance r)
{
  switch (r)
  {
  case Sample::Result::RELEVANCE_IRRELEVANT: return "Irrelevant";
  case Sample::Result::RELEVANCE_RELEVANT: return "Relevant";
  case Sample::Result::RELEVANCE_VITAL: return "Vital";
  }
  return "Unknown";
}

string DebugPrint(Sample::Result const & r)
{
  ostringstream oss;
  oss << "relevance: " << DebugPrint(r.m_relevance) << " ";
  oss << "name: " << DebugPrint(r.m_name) << " ";
  oss << "house number: " << r.m_houseNumber << " ";
  oss << "pos: " << r.m_pos << " ";
  oss << "types: [";
  for (size_t i = 0; i < r.m_types.size(); ++i)
  {
    if (i > 0)
      oss << " ";
    oss << r.m_types[i];
  }
  oss << "]";
  return oss.str();
}

string DebugPrint(Sample const & s)
{
  ostringstream oss;
  oss << "[";
  oss << "query: " << DebugPrint(s.m_query) << " ";
  oss << "locale: " << s.m_locale << " ";
  oss << "pos: " << DebugPrint(s.m_pos) << " ";
  oss << "viewport: " << DebugPrint(s.m_viewport) << " ";
  oss << "results: [";
  for (size_t i = 0; i < s.m_results.size(); ++i)
  {
    if (i > 0)
      oss << " ";
    oss << DebugPrint(s.m_results[i]);
  }
  oss << "]";
  return oss.str();
}
}  // namespace search
