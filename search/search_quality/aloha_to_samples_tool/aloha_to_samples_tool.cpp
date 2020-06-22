#include "search/search_quality/sample.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/search_string_utils.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "3party/gflags/src/gflags/gflags.h"

#define ALOHALYTICS_SERVER
#include "3party/Alohalytics/src/event_base.h"

using namespace search;
using namespace std;
using namespace strings;

DEFINE_string(data_path, "", "Path to data directory (resources dir).");
DEFINE_string(categorial, "all",
              "Allowed values: 'all' - save all requests; 'only' - save only categorial requests; "
              "'no' - save all but categorial requests.");
DEFINE_string(locales, "", "Comma separated locales to filter samples.");
DEFINE_string(mode, "clicked",
              "Allowed values: 'all' - save all searchEmitResultAndCoords event as samples; "
              "'clicked' - save only samples wits corresponding searchShowResult event, add results to samples.");

struct EmitInfo
{
  m2::PointD m_pos;
  m2::RectD m_viewport;
  vector<string> m_results;
  string m_query;
  string m_locale;
  bool m_isCategorial;
};

struct ResultInfo
{
  size_t m_pos;
  string m_result;
};

bool IsCategorialRequest(string query, uint8_t locale)
{
  if (query.empty() || query.back() != ' ')
    return false;

  Trim(query);
  auto const normalizedQuery = NormalizeAndSimplifyString(query);

  auto const & catHolder = GetDefaultCategories();

  bool isCategorialRequest = false;
  catHolder.ForEachName([&](auto const & categorySynonym) {
    if (isCategorialRequest)
      return;

    if (categorySynonym.m_locale != locale &&
        categorySynonym.m_locale != CategoriesHolder::kEnglishCode)
    {
      return;
    }

    auto const normalizedCat = NormalizeAndSimplifyString(categorySynonym.m_name);
    if (normalizedCat == normalizedQuery)
      isCategorialRequest = true;
  });
  return isCategorialRequest;
}

optional<EmitInfo> ParseEmitResultsAndCoords(map<string, string> const & kpePairs)
{
  EmitInfo info;
  try
  {
    if (!to_double(kpePairs.at("posX"), info.m_pos.x))
      return {};
    if (!to_double(kpePairs.at("posY"), info.m_pos.y))
      return {};

    double minX, minY, maxX, maxY;
    bool gotViewport = true;
    gotViewport = gotViewport && to_double(kpePairs.at("viewportMinX"), minX);
    gotViewport = gotViewport && to_double(kpePairs.at("viewportMinY"), minY);
    gotViewport = gotViewport && to_double(kpePairs.at("viewportMaxX"), maxX);
    gotViewport = gotViewport && to_double(kpePairs.at("viewportMaxY"), maxY);
    if (!gotViewport)
      return {};
    info.m_viewport = m2::RectD(minX, minY, maxX, maxY);

    info.m_locale = kpePairs.at("locale");
    info.m_query = kpePairs.at("query");

    auto const locale = CategoriesHolder::MapLocaleToInteger(info.m_locale);
    info.m_isCategorial = IsCategorialRequest(info.m_query, locale);

    info.m_results = Tokenize(kpePairs.at("results"), "\t");
    // Skip results number.
    if (!info.m_results.empty())
      info.m_results.erase(info.m_results.begin());
  }
  catch (out_of_range)
  {
    return {};
  }

  return info;
}

optional<ResultInfo> ParseShowResult(map<string, string> const & kpePairs)
{
  ResultInfo info;
  try
  {
    if (!to_size_t(kpePairs.at("pos"), info.m_pos))
      return {};
    info.m_result = kpePairs.at("result");
  }
  catch (out_of_range)
  {
    return {};
  }

  return info;
}

optional<Sample::Result> ParseResultWithCoords(string const & str)
{
  auto const tokens = Tokenize(str, "|");
  // No coords.
  if (tokens.size() < 5)
    return {};

  // Suggest.
  if (tokens[2] == "1")
    return {};

  Sample::Result res;
  res.m_name = MakeUniString(tokens[0]);
  res.m_types = {tokens[1]};

  double lat;
  if (!to_double(tokens[3], lat))
    return {};

  double lon;
  if (!to_double(tokens[4], lon))
    return {};

  res.m_pos = mercator::FromLatLon(lat, lon);

  return res;
}

optional<Sample> MakeSample(EmitInfo const & info, optional<size_t> relevantPos)
{
  Sample sample;
  sample.m_query = MakeUniString(info.m_query);
  sample.m_locale = info.m_locale;
  sample.m_pos = info.m_pos;
  sample.m_viewport = info.m_viewport;
  sample.m_results.reserve(info.m_results.size());
  if (!relevantPos)
    return sample;

  for (size_t i = 0; i < info.m_results.size(); ++i)
  {
    auto res = ParseResultWithCoords(info.m_results[i]);
    if (!res)
      return {};

    res->m_relevance = i == relevantPos ? Sample::Result::Relevance::Relevant
                                        : Sample::Result::Relevance::Irrelevant;
    sample.m_results.push_back(*res);
  }
  return sample;
}

void PrintSample(EmitInfo const & info, optional<size_t> relevantPos)
{
  auto const sample = MakeSample(info, relevantPos);
  if (!sample)
    return;

  string json;
  Sample::SerializeToJSONLines({*sample}, json);
  cout << json;
}

int main(int argc, char * argv[])
{
  google::SetUsageMessage("This tool converts events from Alohalytics to search samples.");
  google::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_categorial != "all" && FLAGS_categorial != "only" && FLAGS_categorial != "no")
  {
    LOG(LINFO, ("Invalid categorial filter mode:", FLAGS_categorial,
                "allowed walues are: 'all', 'only', 'no'."));
    return EXIT_FAILURE;
  }

  if (FLAGS_mode != "all" && FLAGS_mode != "clicked")
  {
    LOG(LINFO, ("Invalid mode:", FLAGS_mode, "allowed walues are: 'all', 'clicked'."));
    return EXIT_FAILURE;
  }

  if (!FLAGS_data_path.empty())
  {
    Platform & platform = GetPlatform();
    platform.SetResourceDir(FLAGS_data_path);
    platform.SetWritableDirForTests(FLAGS_data_path);
  }

  classificator::Load();

  cereal::BinaryInputArchive ar(cin);
  unique_ptr<AlohalyticsBaseEvent> ptr;
  bool newUser = true;
  optional<EmitInfo> info;

  auto const locales = strings::Tokenize<set>(FLAGS_locales, ",");

  while (true)
  {
    try
    {
      ar(ptr);
    }
    catch (const cereal::Exception & ex)
    {
      if (string("Failed to read 4 bytes from input stream! Read 0") != ex.what())
      {
        // The exception above is a normal one, Cereal lacks to detect the end of the stream.
        cerr << ex.what() << endl;
        return EXIT_FAILURE;
      }
      return EXIT_SUCCESS;
    }

    auto const * event = dynamic_cast<AlohalyticsIdServerEvent const *>(ptr.get());
    if (event)
    {
      newUser = true;
      continue;
    }

    auto const * kpe = dynamic_cast<AlohalyticsKeyPairsEvent const *>(ptr.get());
    if (!kpe)
      continue;

    if (kpe->key == "searchEmitResultsAndCoords")
    {
      info = ParseEmitResultsAndCoords(kpe->pairs);
      newUser = false;

      if (FLAGS_mode == "all")
        PrintSample(*info, {});
    }
    else if (kpe->key == "searchShowResult" && !newUser)
    {
      auto const result = ParseShowResult(kpe->pairs);
      if (!info || !result)
        continue;

      if ((FLAGS_categorial == "only" && !info->m_isCategorial) ||
          (FLAGS_categorial == "no" && info->m_isCategorial))
      {
        continue;
      }

      if (!locales.empty() && locales.count(info->m_locale) == 0)
        continue;

      auto const resultMatches = info->m_results.size() > result->m_pos &&
                                 info->m_results[result->m_pos] == result->m_result;
      if (!resultMatches)
        continue;

      PrintSample(*info, result->m_pos);
    }
  }
  return EXIT_SUCCESS;
}
