#include "search/ranking_info.hpp"
#include "search/result.hpp"
#include "search/search_quality/helpers.hpp"
#include "search/search_quality/sample.hpp"
#include "search/search_tests_support/test_search_engine.hpp"
#include "search/search_tests_support/test_search_request.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/feature_algo.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/index.hpp"
#include "storage/storage.hpp"

#include "coding/file_name_utils.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "base/macros.hpp"
#include "base/string_utils.hpp"

#include "std/fstream.hpp"
#include "std/iostream.hpp"
#include "std/limits.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

#include "defines.hpp"

#include "3party/gflags/src/gflags/gflags.h"

using namespace search::tests_support;
using namespace search;
using namespace storage;

DEFINE_string(data_path, "", "Path to data directory (resources dir)");
DEFINE_string(mwm_path, "", "Path to mwm files (writable dir)");
DEFINE_string(stats_path, "", "Path to store stats about queries results (default: stderr)");
DEFINE_string(json_in, "", "Path to the json file with samples (default: stdin)");

size_t constexpr kInvalidId = numeric_limits<size_t>::max();

struct Stats
{
  // Indexes of not-found VITAL or RELEVANT results.
  vector<size_t> m_notFound;
};

struct Context
{
  Context(Index & index) : m_index(index) {}

  WARN_UNUSED_RESULT bool GetFeature(FeatureID const & id, FeatureType & ft)
  {
    auto const & mwmId = id.m_mwmId;
    if (!m_guard || m_guard->GetId() != mwmId)
      m_guard = make_unique<Index::FeaturesLoaderGuard>(m_index, mwmId);
    return m_guard->GetFeatureByIndex(id.m_index, ft);
  }

  Index & m_index;
  unique_ptr<Index::FeaturesLoaderGuard> m_guard;
};

void GetContents(istream & is, string & contents)
{
  string line;
  while (getline(is, line))
  {
    contents.append(line);
    contents.push_back('\n');
  }
}

bool Matches(Context & context, Sample::Result const & golden, search::Result const & actual)
{
  static double constexpr kToleranceMeters = 50;
  if (actual.GetResultType() != Result::RESULT_FEATURE)
    return false;

  FeatureType ft;
  if (!context.GetFeature(actual.GetFeatureID(), ft))
    return false;

  auto const houseNumber = ft.GetHouseNumber();
  auto const center = feature::GetCenter(ft);

  bool nameMatches = false;
  if (golden.m_name.empty())
  {
    nameMatches = true;
  }
  else
  {
    ft.ForEachName([&golden, &nameMatches](int8_t /* lang */, string const & name) {
      if (golden.m_name == strings::MakeUniString(name))
      {
        nameMatches = true;
        return false;  // breaks the loop
      }
      return true;  // continues the loop
    });
  }

  return nameMatches && golden.m_houseNumber == houseNumber &&
         MercatorBounds::DistanceOnEarth(golden.m_pos, center) < kToleranceMeters;
}

void MatchResults(Context & context, vector<Sample::Result> const & golden,
                  vector<search::Result> const & actual, vector<size_t> & goldenMatching,
                  vector<size_t> & actualMatching)
{
  auto const n = golden.size();
  auto const m = actual.size();

  goldenMatching.assign(n, kInvalidId);
  actualMatching.assign(m, kInvalidId);

  // TODO (@y, @m): use Kuhn algorithm here for maximum matching.
  for (size_t i = 0; i < n; ++i)
  {
    if (goldenMatching[i] != kInvalidId)
      continue;
    auto const & g = golden[i];

    for (size_t j = 0; j < m; ++j)
    {
      if (actualMatching[j] != kInvalidId)
        continue;

      auto const & a = actual[j];
      if (Matches(context, g, a))
      {
        goldenMatching[i] = j;
        actualMatching[j] = i;
        break;
      }
    }
  }
}

void DidDownload(TCountryId const & /* countryId */,
                 shared_ptr<platform::LocalCountryFile> const & /* localFile */)
{
}

bool WillDelete(TCountryId const & /* countryId */,
                shared_ptr<platform::LocalCountryFile> const & /* localFile */)
{
  return false;
}

void DisplayStats(ostream & os, vector<Sample> const & samples, vector<Stats> const & stats)
{
  auto const n = samples.size();
  ASSERT_EQUAL(stats.size(), n, ());

  size_t numWarnings = 0;
  for (auto const & stat : stats)
  {
    if (!stat.m_notFound.empty())
      ++numWarnings;
  }

  if (numWarnings == 0)
  {
    os << "All " << stats.size() << " queries are OK." << endl;
    return;
  }

  os << numWarnings << " warnings." << endl;
  for (size_t i = 0; i < n; ++i)
  {
    if (stats[i].m_notFound.empty())
      continue;
    os << "Query #" << i << " \"" << strings::ToUtf8(samples[i].m_query) << "\":" << endl;
    for (auto const & j : stats[i].m_notFound)
      os << "Not found: " << DebugPrint(samples[i].m_results[j]) << endl;
  }
}

int main(int argc, char * argv[])
{
  ChangeMaxNumberOfOpenFiles(kMaxOpenFiles);

  google::SetUsageMessage("Features collector tool.");
  google::ParseCommandLineFlags(&argc, &argv, true);

  Platform & platform = GetPlatform();

  string countriesFile = COUNTRIES_FILE;
  if (!FLAGS_data_path.empty())
  {
    platform.SetResourceDir(FLAGS_data_path);
    countriesFile = my::JoinFoldersToPath(FLAGS_data_path, COUNTRIES_FILE);
  }

  if (!FLAGS_mwm_path.empty())
    platform.SetWritableDirForTests(FLAGS_mwm_path);

  LOG(LINFO, ("writable dir =", platform.WritableDir()));
  LOG(LINFO, ("resources dir =", platform.ResourcesDir()));

  Storage storage(countriesFile, FLAGS_mwm_path);
  storage.Init(&DidDownload, &WillDelete);
  auto infoGetter = CountryInfoReader::CreateCountryInfoReader(platform);
  infoGetter->InitAffiliationsInfo(&storage.GetAffiliations());

  string jsonStr;
  if (FLAGS_json_in.empty())
  {
    GetContents(cin, jsonStr);
  }
  else
  {
    ifstream ifs(FLAGS_json_in);
    if (!ifs.is_open())
    {
      cerr << "Can't open input json file." << endl;
      return -1;
    }
    GetContents(ifs, jsonStr);
  }

  vector<Sample> samples;
  if (!Sample::DeserializeFromJSON(jsonStr, samples))
  {
    cerr << "Can't parse input json file." << endl;
    return -1;
  }

  classificator::Load();
  TestSearchEngine engine(move(infoGetter), make_unique<ProcessorFactory>(), Engine::Params{});

  vector<platform::LocalCountryFile> mwms;
  platform::FindAllLocalMapsAndCleanup(numeric_limits<int64_t>::max() /* the latest version */,
                                       mwms);
  for (auto & mwm : mwms)
  {
    mwm.SyncWithDisk();
    engine.RegisterMap(mwm);
  }

  vector<Stats> stats(samples.size());
  Context context(engine);

  cout << "SampleId,";
  RankingInfo::PrintCSVHeader(cout);
  cout << ",Relevance" << endl;

  for (size_t i = 0; i < samples.size(); ++i)
  {
    auto const & sample = samples[i];

    engine.SetLocale(sample.m_locale);

    auto latLon = MercatorBounds::ToLatLon(sample.m_pos);

    search::SearchParams params;
    params.m_query = strings::ToUtf8(sample.m_query);
    params.m_inputLocale = sample.m_locale;
    params.m_mode = Mode::Everywhere;
    params.SetPosition(latLon.lat, latLon.lon);
    params.m_suggestsEnabled = false;
    TestSearchRequest request(engine, params, sample.m_viewport);
    request.Run();

    auto const & results = request.Results();

    vector<size_t> goldenMatching;
    vector<size_t> actualMatching;
    MatchResults(context, sample.m_results, results, goldenMatching, actualMatching);

    for (size_t j = 0; j < results.size(); ++j)
    {
      if (results[j].GetResultType() != Result::RESULT_FEATURE)
        continue;
      auto const & info = results[j].GetRankingInfo();
      cout << i << ",";
      info.ToCSV(cout);

      auto relevance = Sample::Result::RELEVANCE_IRRELEVANT;
      if (actualMatching[j] != kInvalidId)
        relevance = sample.m_results[actualMatching[j]].m_relevance;
      cout << "," << DebugPrint(relevance) << endl;
    }

    auto & s = stats[i];
    for (size_t j = 0; j < goldenMatching.size(); ++j)
    {
      if (goldenMatching[j] == kInvalidId &&
          sample.m_results[j].m_relevance != Sample::Result::RELEVANCE_IRRELEVANT)
      {
        s.m_notFound.push_back(j);
      }
    }
  }

  if (FLAGS_stats_path.empty())
  {
    DisplayStats(cerr, samples, stats);
  }
  else
  {
    ofstream ofs(FLAGS_stats_path);
    if (!ofs.is_open())
    {
      cerr << "Can't open output file for stats." << endl;
      return -1;
    }
    DisplayStats(ofs, samples, stats);
  }
  return 0;
}
