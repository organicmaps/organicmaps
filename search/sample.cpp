#include "sample.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/sstream.hpp"
#include "std/string.hpp"

#include "3party/jansson/myjansson.hpp"

namespace
{
void FromJSONObject(json_t * root, string & result) { result = string(json_string_value(root)); }

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

void FromJSONObject(json_t * root, string const & field, vector<string> & result)
{
  json_t * arr = json_object_get(root, field.c_str());
  if (!arr)
    MYTHROW(my::Json::Exception, ("Obligatory field", field, "is absent."));
  if (!json_is_array(arr))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json array."));
  size_t const sz = json_array_size(arr);
  result.resize(sz);
  for (size_t i = 0; i < sz; ++i)
  {
    json_t * elem = json_array_get(arr, i);
    if (!json_is_string(elem))
      MYTHROW(my::Json::Exception, ("Wrong type:", field, "must contain an array of strings."));
    result[i] = json_string_value(elem);
  }
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

void FromJSONObject(json_t * root, search::Sample::Result & result)
{
  FromJSONObject(root, "position", result.m_pos);
  FromJSONObject(root, "name", result.m_name);
  FromJSONObject(root, "houseNumber", result.m_houseNumber);
  FromJSONObject(root, "types", result.m_types);
  FromJSONObject(root, "relevancy", result.m_relevance);
}

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
    FromJSONObject(json_array_get(arr, i), result[i]);
}
}  // namespace

namespace search
{
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

string Sample::ToStringDebug() const
{
  ostringstream oss;
  oss << "[";
  oss << "query: " << DebugPrint(m_query) << " ";
  oss << "locale: " << m_locale << " ";
  oss << "pos: " << DebugPrint(m_pos) << " ";
  oss << "viewport: " << DebugPrint(m_viewport) << " ";
  oss << "results: [";
  for (size_t i = 0; i < m_results.size(); ++i)
  {
    if (i > 0)
      oss << " ";
    oss << DebugPrint(m_results[i]);
  }
  oss << "]";
  return oss.str();
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

string DebugPrint(Sample const & s) { return s.ToStringDebug(); }
}  // namespace search
