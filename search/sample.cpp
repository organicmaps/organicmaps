#include "sample.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/sstream.hpp"
#include "std/string.hpp"

#include "3party/jansson/myjansson.hpp"

namespace
{
void ParsePosition(json_t * json, m2::PointD & pos)
{
  if (!json_is_object(json))
    MYTHROW(my::Json::Exception, ("Position must be a json object."));

  json_t * posX = json_object_get(json, "x");
  json_t * posY = json_object_get(json, "y");
  if (!posX || !posY)
    MYTHROW(my::Json::Exception, ("Position must have both x and y set."));
  if (!json_is_number(posX) || !json_is_number(posY))
    MYTHROW(my::Json::Exception, ("Position's x and y must be numbers."));
  pos.x = json_number_value(posX);
  pos.y = json_number_value(posY);
}

string ParseObligatoryString(json_t * root, string const & fieldName)
{
  json_t * str = json_object_get(root, fieldName.c_str());
  if (!str)
    MYTHROW(my::Json::Exception, ("Obligatory field", fieldName, "is absent."));
  if (!json_is_string(str))
    MYTHROW(my::Json::Exception, ("The field", fieldName, "must contain a json string."));
  return string(json_string_value(str));
}

void ParseResult(json_t * root, search::Sample::Result & result)
{
  json_t * position = json_object_get(root, "position");
  if (!position)
    MYTHROW(my::Json::Exception, ("Obligatory field", "position", "is absent."));
  ParsePosition(position, result.m_pos);

  result.m_name = strings::MakeUniString(ParseObligatoryString(root, "name"));

  result.m_houseNumber = ParseObligatoryString(root, "houseNumber");

  json_t * types = json_object_get(root, "types");
  if (!types)
    MYTHROW(my::Json::Exception, ("Obligatory field", "types", "is absent."));
  if (!json_is_array(types))
    MYTHROW(my::Json::Exception, ("The field", "types", "must contain a json array."));
  size_t const numTypes = json_array_size(types);
  result.m_types.resize(numTypes);
  for (size_t i = 0; i < numTypes; ++i)
  {
    json_t * type = json_array_get(types, i);
    if (!json_is_string(type))
      MYTHROW(my::Json::Exception, ("Result types must be strings."));
    result.m_types[i] = json_string_value(type);
  }

  string relevance = ParseObligatoryString(root, "relevancy");
  if (relevance == "vital")
    result.m_relevance = search::Sample::Result::Relevance::RELEVANCE_VITAL;
  else if (relevance == "relevant")
    result.m_relevance = search::Sample::Result::Relevance::RELEVANCE_RELEVANT;
  else
    result.m_relevance = search::Sample::Result::Relevance::RELEVANCE_IRRELEVANT;
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
  m_query = strings::MakeUniString(ParseObligatoryString(root, "query"));
  m_locale = ParseObligatoryString(root, "locale");

  json_t * position = json_object_get(root, "position");
  if (!position)
    MYTHROW(my::Json::Exception, ("Obligatory field", "position", "is absent."));
  ParsePosition(position, m_pos);

  json_t * viewport = json_object_get(root, "viewport");
  if (!viewport)
    MYTHROW(my::Json::Exception, ("Obligatory field", "viewport", "is absent."));
  m_viewport.setMinX(json_number_value(json_object_get(viewport, "minx")));
  m_viewport.setMinY(json_number_value(json_object_get(viewport, "miny")));
  m_viewport.setMaxX(json_number_value(json_object_get(viewport, "maxx")));
  m_viewport.setMaxY(json_number_value(json_object_get(viewport, "maxy")));

  json_t * results = json_object_get(root, "results");
  if (!results)
    MYTHROW(my::Json::Exception, ("Obligatory field", "results", "is absent."));
  if (!json_is_array(results))
    MYTHROW(my::Json::Exception, ("The field", "results", "must contain a json array."));
  size_t numResults = json_array_size(results);
  m_results.resize(numResults);
  for (size_t i = 0; i < numResults; ++i)
    ParseResult(json_array_get(results, i), m_results[i]);
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
