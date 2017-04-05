#include "search/search_quality/sample.hpp"

#include "search/search_quality/helpers.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_data.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>

using namespace my;

namespace
{
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
bool Less(std::vector<T> lhs, std::vector<T> rhs)
{
  sort(lhs.begin(), lhs.end());
  sort(rhs.begin(), rhs.end());
  return lhs < rhs;
}

template <typename T>
bool Equal(std::vector<T> lhs, std::vector<T> rhs)
{
  sort(lhs.begin(), lhs.end());
  sort(rhs.begin(), rhs.end());
  return lhs == rhs;
}

struct FreeDeletor
{
  void operator()(char * buffer) const { free(buffer); }
};
}  // namespace

namespace search
{
// static
Sample::Result Sample::Result::Build(FeatureType & ft, Relevance relevance)
{
  Sample::Result r;
  r.m_pos = feature::GetCenter(ft);
  {
    string name;
    ft.GetReadableName(false /* allowTranslit */, name);
    r.m_name = strings::MakeUniString(name);
  }
  r.m_houseNumber = ft.GetHouseNumber();
  r.m_types = feature::TypesHolder(ft).ToObjectNames();
  r.m_relevance = relevance;
  return r;
}

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

my::JSONPtr Sample::SerializeToJSON() const
{
  auto json = my::NewJSONObject();
  SerializeToJSONImpl(*json);
  return json;
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
bool Sample::DeserializeFromJSON(string const & jsonStr, std::vector<Sample> & samples)
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

// static
void Sample::SerializeToJSON(std::vector<Sample> const & samples, std::string & jsonStr)
{
  auto array = my::NewJSONArray();
  for (auto const & sample : samples)
    json_array_append_new(array.get(), sample.SerializeToJSON().release());
  std::unique_ptr<char, FreeDeletor> buffer(
      json_dumps(array.get(), JSON_COMPACT | JSON_ENSURE_ASCII));
  jsonStr.assign(buffer.get());
}

void Sample::DeserializeFromJSONImpl(json_t * root)
{
  FromJSONObject(root, "query", m_query);
  FromJSONObject(root, "locale", m_locale);
  FromJSONObject(root, "position", m_pos);
  FromJSONObject(root, "viewport", m_viewport);
  FromJSONObject(root, "results", m_results);
}

void Sample::SerializeToJSONImpl(json_t & root) const
{
  ToJSONObject(root, "query", m_query);
  ToJSONObject(root, "locale", m_locale);
  ToJSONObject(root, "position", m_pos);
  ToJSONObject(root, "viewport", m_viewport);
  ToJSONObject(root, "results", m_results);
}

void FromJSONObject(json_t * root, string const & field, Sample::Result::Relevance & relevance)
{
  string r;
  FromJSONObject(root, field, r);
  if (r == "vital")
    relevance = search::Sample::Result::Relevance::Vital;
  else if (r == "relevant")
    relevance = search::Sample::Result::Relevance::Relevant;
  else if (r == "irrelevant")
    relevance = search::Sample::Result::Relevance::Irrelevant;
  else
    CHECK(false, ("Unknown relevance:", r));
}

void ToJSONObject(json_t & root, string const & field, Sample::Result::Relevance relevance)
{
  using Relevance = Sample::Result::Relevance;

  string r;
  switch (relevance)
  {
  case Relevance::Vital: r = "vital"; break;
  case Relevance::Relevant: r = "relevant"; break;
  case Relevance::Irrelevant: r = "irrelevant"; break;
  }

  json_object_set_new(&root, field.c_str(), json_string(r.c_str()));
}

void FromJSON(json_t * root, Sample::Result & result)
{
  FromJSONObject(root, "position", result.m_pos);
  FromJSONObject(root, "name", result.m_name);
  FromJSONObject(root, "houseNumber", result.m_houseNumber);
  FromJSONObject(root, "types", result.m_types);
  FromJSONObject(root, "relevancy", result.m_relevance);
}

my::JSONPtr ToJSON(Sample::Result const & result)
{
  auto root = my::NewJSONObject();
  ToJSONObject(*root, "position", result.m_pos);
  ToJSONObject(*root, "name", result.m_name);
  ToJSONObject(*root, "houseNumber", result.m_houseNumber);
  ToJSONObject(*root, "types", result.m_types);
  ToJSONObject(*root, "relevancy", result.m_relevance);
  return root;
}

string DebugPrint(Sample::Result::Relevance r)
{
  switch (r)
  {
  case Sample::Result::Relevance::Irrelevant: return "Irrelevant";
  case Sample::Result::Relevance::Relevant: return "Relevant";
  case Sample::Result::Relevance::Vital: return "Vital";
  }
  return "Unknown";
}

string DebugPrint(Sample::Result const & r)
{
  ostringstream oss;
  oss << "relevance: " << DebugPrint(r.m_relevance) << " ";
  oss << "name: " << DebugPrint(r.m_name) << " ";
  oss << "house number: " << r.m_houseNumber << " ";
  oss << "pos: " << DebugPrint(r.m_pos) << " ";
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
