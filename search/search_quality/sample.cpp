#include "search/search_quality/sample.hpp"

#include "search/search_params.hpp"
#include "search/search_quality/helpers.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_data.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <ios>
#include <memory>
#include <sstream>
#include <string>

using namespace my;
using namespace std;

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
    ft.GetReadableName(name);
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
    LOG(LWARNING, ("Can't parse sample:", e.Msg(), jsonStr));
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
  if (m_posAvailable != rhs.m_posAvailable)
    return m_posAvailable < rhs.m_posAvailable;
  if (m_viewport != rhs.m_viewport)
    return LessRect(m_viewport, rhs.m_viewport);
  if (!Equal(m_results, rhs.m_results))
    return Less(m_results, rhs.m_results);
  return Less(m_relatedQueries, rhs.m_relatedQueries);
}

bool Sample::operator==(Sample const & rhs) const { return !(*this < rhs) && !(rhs < *this); }

// static
bool Sample::DeserializeFromJSONLines(string const & lines, std::vector<Sample> & samples)
{
  istringstream is(lines);
  string line;
  vector<Sample> result;

  while (getline(is, line))
  {
    if (line.empty())
      continue;

    Sample sample;
    if (!sample.DeserializeFromJSON(line))
      return false;
    result.emplace_back(move(sample));
  }

  samples.insert(samples.end(), result.begin(), result.end());
  return true;
}

// static
void Sample::SerializeToJSONLines(std::vector<Sample> const & samples, std::string & lines)
{
  for (auto const & sample : samples)
  {
    unique_ptr<char, JSONFreeDeleter> buffer(
        json_dumps(sample.SerializeToJSON().get(), JSON_COMPACT | JSON_ENSURE_ASCII));
    lines.append(buffer.get());
    lines.push_back('\n');
  }
}

void Sample::DeserializeFromJSONImpl(json_t * root)
{
  FromJSONObject(root, "query", m_query);
  FromJSONObject(root, "locale", m_locale);

  m_posAvailable = FromJSONObjectOptional(root, "position", m_pos);

  FromJSONObject(root, "viewport", m_viewport);
  FromJSONObjectOptional(root, "results", m_results);
  FromJSONObjectOptional(root, "related_queries", m_relatedQueries);
}

void Sample::SerializeToJSONImpl(json_t & root) const
{
  ToJSONObject(root, "query", m_query);
  ToJSONObject(root, "locale", m_locale);
  ToJSONObject(root, "position", m_pos);
  ToJSONObject(root, "viewport", m_viewport);
  ToJSONObject(root, "results", m_results);
  ToJSONObject(root, "related_queries", m_relatedQueries);
}

void Sample::FillSearchParams(search::SearchParams & params) const
{
  params.m_query = strings::ToUtf8(m_query);
  params.m_inputLocale = m_locale;
  params.m_viewport = m_viewport;
  params.m_mode = Mode::Everywhere;
  if (m_posAvailable)
    params.m_position = m_pos;

  params.m_needAddress = true;
  params.m_suggestsEnabled = false;
  params.m_needHighlighting = false;
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
  oss << "query: " << DebugPrint(s.m_query) << ", ";
  oss << "locale: " << s.m_locale << ", ";
  oss << "pos: " << DebugPrint(s.m_pos) << ", ";
  oss << "posAvailable: " << boolalpha << s.m_posAvailable << ", ";
  oss << "viewport: " << DebugPrint(s.m_viewport) << ", ";
  oss << "results: [";
  for (size_t i = 0; i < s.m_results.size(); ++i)
  {
    if (i > 0)
      oss << ", ";
    oss << DebugPrint(s.m_results[i]);
  }
  oss << "]";
  return oss.str();
}
}  // namespace search
