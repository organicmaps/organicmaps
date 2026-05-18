#include "search/search_quality/sample.hpp"

#include "search/search_params.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_data.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <glaze/json.hpp>

#include <algorithm>
#include <sstream>

namespace search
{
namespace sample_json
{
struct Point
{
  double x = 0.0;
  double y = 0.0;
};

struct Rect
{
  double minx = 0.0;
  double miny = 0.0;
  double maxx = 0.0;
  double maxy = 0.0;
};

struct Result
{
  Point position;
  std::string name;
  std::string houseNumber;
  std::vector<std::string> types;
  std::string relevancy;
};

struct Sample
{
  std::string query;
  std::string locale;
  std::optional<Point> position;
  Rect viewport;
  std::optional<std::vector<Result>> results;
  std::optional<std::vector<std::string>> related_queries;
  std::optional<bool> useless;
};
}  // namespace sample_json

namespace
{
sample_json::Point ToJson(m2::PointD const & point)
{
  return {.x = point.x, .y = point.y};
}

m2::PointD FromJson(sample_json::Point const & point)
{
  return {point.x, point.y};
}

sample_json::Rect ToJson(m2::RectD const & rect)
{
  return {.minx = rect.minX(), .miny = rect.minY(), .maxx = rect.maxX(), .maxy = rect.maxY()};
}

m2::RectD FromJson(sample_json::Rect const & rect)
{
  return {rect.minx, rect.miny, rect.maxx, rect.maxy};
}

std::string ToJson(Sample::Result::Relevance relevance)
{
  using Relevance = Sample::Result::Relevance;
  switch (relevance)
  {
  case Relevance::Harmful: return "harmful";
  case Relevance::Irrelevant: return "irrelevant";
  case Relevance::Relevant: return "relevant";
  case Relevance::Vital: return "vital";
  }
  return {};
}

bool FromJson(std::string const & relevance, Sample::Result::Relevance & result)
{
  if (relevance == "harmful")
  {
    result = Sample::Result::Relevance::Harmful;
    return true;
  }
  if (relevance == "irrelevant")
  {
    result = Sample::Result::Relevance::Irrelevant;
    return true;
  }
  if (relevance == "relevant")
  {
    result = Sample::Result::Relevance::Relevant;
    return true;
  }
  if (relevance == "vital")
  {
    result = Sample::Result::Relevance::Vital;
    return true;
  }
  return false;
}

sample_json::Result ToJson(Sample::Result const & result)
{
  return {.position = ToJson(result.m_pos),
          .name = strings::ToUtf8(result.m_name),
          .houseNumber = result.m_houseNumber,
          .types = result.m_types,
          .relevancy = ToJson(result.m_relevance)};
}

bool FromJson(sample_json::Result const & json, Sample::Result & result)
{
  result.m_pos = FromJson(json.position);
  result.m_name = strings::MakeUniString(json.name);
  result.m_houseNumber = json.houseNumber;
  result.m_types = json.types;
  return FromJson(json.relevancy, result.m_relevance);
}

sample_json::Sample ToJson(Sample const & sample)
{
  sample_json::Sample json;
  json.query = strings::ToUtf8(sample.m_query);
  json.locale = sample.m_locale;
  if (sample.m_pos)
    json.position = ToJson(*sample.m_pos);
  json.viewport = ToJson(sample.m_viewport);
  json.results = std::vector<sample_json::Result>();
  json.results->reserve(sample.m_results.size());
  for (auto const & result : sample.m_results)
    json.results->push_back(ToJson(result));
  json.related_queries = std::vector<std::string>();
  json.related_queries->reserve(sample.m_relatedQueries.size());
  for (auto const & query : sample.m_relatedQueries)
    json.related_queries->push_back(strings::ToUtf8(query));
  if (sample.m_useless)
    json.useless = sample.m_useless;
  return json;
}

bool FromJson(sample_json::Sample const & json, Sample & sample)
{
  sample.m_query = strings::MakeUniString(json.query);
  sample.m_locale = json.locale;
  sample.m_pos = json.position ? std::make_optional(FromJson(*json.position)) : std::nullopt;
  sample.m_viewport = FromJson(json.viewport);
  sample.m_results.clear();
  if (json.results)
  {
    sample.m_results.reserve(json.results->size());
    for (auto const & resultJson : *json.results)
    {
      Sample::Result result;
      if (!FromJson(resultJson, result))
        return false;
      sample.m_results.push_back(std::move(result));
    }
  }
  sample.m_relatedQueries.clear();
  if (json.related_queries)
  {
    sample.m_relatedQueries.reserve(json.related_queries->size());
    for (auto const & query : *json.related_queries)
      sample.m_relatedQueries.push_back(strings::MakeUniString(query));
  }
  sample.m_useless = json.useless.value_or(false);
  return true;
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
bool Less(std::vector<T> lhs, std::vector<T> rhs)
{
  std::sort(lhs.begin(), lhs.end());
  std::sort(rhs.begin(), rhs.end());
  return lhs < rhs;
}

template <typename T>
bool Equal(std::vector<T> lhs, std::vector<T> rhs)
{
  std::sort(lhs.begin(), lhs.end());
  std::sort(rhs.begin(), rhs.end());
  return lhs == rhs;
}
}  // namespace

// static
Sample::Result Sample::Result::Build(FeatureType & ft, Relevance relevance)
{
  Sample::Result r;
  r.m_pos = feature::GetCenter(ft);
  r.m_name = strings::MakeUniString(ft.GetReadableName());
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

bool Sample::DeserializeFromJSON(std::string const & jsonStr)
{
  sample_json::Sample sampleJson;
  glz::opts constexpr opts{.error_on_unknown_keys = false};
  if (auto const error = glz::read<opts>(sampleJson, jsonStr); error)
  {
    LOG(LWARNING, ("Can't parse sample:", glz::format_error(error, jsonStr), jsonStr));
    return false;
  }

  if (!FromJson(sampleJson, *this))
  {
    LOG(LWARNING, ("Can't parse sample: unknown relevance", jsonStr));
    return false;
  }

  return true;
}

std::string Sample::SerializeToJSON() const
{
  std::string buffer;
  auto json = ToJson(*this);
  if (auto const error = glz::write_json(json, buffer); error)
  {
    LOG(LWARNING, ("Can't serialize sample:", glz::format_error(error)));
    return {};
  }
  return buffer;
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
  if (!Equal(m_results, rhs.m_results))
    return Less(m_results, rhs.m_results);
  return Less(m_relatedQueries, rhs.m_relatedQueries);
}

bool Sample::operator==(Sample const & rhs) const
{
  return !(*this < rhs) && !(rhs < *this);
}

// static
bool Sample::DeserializeFromJSONLines(std::string const & lines, std::vector<Sample> & samples)
{
  std::istringstream is(lines);
  std::string line;
  std::vector<Sample> result;

  while (std::getline(is, line))
  {
    if (line.empty())
      continue;

    Sample sample;
    if (!sample.DeserializeFromJSON(line))
      return false;
    result.emplace_back(std::move(sample));
  }

  samples.insert(samples.end(), result.begin(), result.end());
  return true;
}

// static
void Sample::SerializeToJSONLines(std::vector<Sample> const & samples, std::string & lines)
{
  for (auto const & sample : samples)
  {
    lines.append(sample.SerializeToJSON());
    lines.push_back('\n');
  }
}

void Sample::FillSearchParams(search::SearchParams & params) const
{
  params.m_query = strings::ToUtf8(m_query);
  params.m_inputLocale = m_locale;
  params.m_viewport = m_viewport;
  params.m_mode = Mode::Everywhere;
  params.m_position = m_pos.value_or(m2::PointD());
  params.m_needAddress = true;
  params.m_suggestsEnabled = false;
  params.m_needHighlighting = false;
  params.m_useDebugInfo = true;  // for RankingInfo printing
}

std::string DebugPrint(Sample::Result::Relevance r)
{
  switch (r)
  {
  case Sample::Result::Relevance::Harmful: return "Harmful";
  case Sample::Result::Relevance::Irrelevant: return "Irrelevant";
  case Sample::Result::Relevance::Relevant: return "Relevant";
  case Sample::Result::Relevance::Vital: return "Vital";
  }
  return "Unknown";
}

std::string DebugPrint(Sample::Result const & r)
{
  std::ostringstream oss;
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

std::string DebugPrint(Sample const & s)
{
  std::ostringstream oss;
  oss << "[";
  oss << "query: " << DebugPrint(s.m_query) << ", ";
  oss << "locale: " << s.m_locale << ", ";
  oss << "pos: " << (s.m_pos ? DebugPrint(*s.m_pos) : "null") << ", ";
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
