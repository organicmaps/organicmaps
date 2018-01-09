#include "search/feature_loader.hpp"
#include "search/ranking_info.hpp"
#include "search/result.hpp"
#include "search/search_quality/helpers.hpp"
#include "search/search_quality/matcher.hpp"
#include "search/search_quality/sample.hpp"
#include "search/search_tests_support/test_search_engine.hpp"
#include "search/search_tests_support/test_search_request.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/index.hpp"

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

struct Stats
{
  // Indexes of not-found VITAL or RELEVANT results.
  vector<size_t> m_notFound;
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
    os << "Query #" << i + 1 << " \"" << strings::ToUtf8(samples[i].m_query) << "\":" << endl;
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

  Storage storage(countriesFile);
  storage.Init(&DidDownload, &WillDelete);
  auto infoGetter = CountryInfoReader::CreateCountryInfoReader(platform);
  infoGetter->InitAffiliationsInfo(&storage.GetAffiliations());

  string lines;
  if (FLAGS_json_in.empty())
  {
    GetContents(cin, lines);
  }
  else
  {
    ifstream ifs(FLAGS_json_in);
    if (!ifs.is_open())
    {
      cerr << "Can't open input json file." << endl;
      return -1;
    }
    GetContents(ifs, lines);
  }

  vector<Sample> samples;
  if (!Sample::DeserializeFromJSONLines(lines, samples))
  {
    cerr << "Can't parse input json file." << endl;
    return -1;
  }

  classificator::Load();
  Index index;

  vector<platform::LocalCountryFile> mwms;
  platform::FindAllLocalMapsAndCleanup(numeric_limits<int64_t>::max() /* the latest version */,
                                       mwms);
  for (auto & mwm : mwms)
  {
    mwm.SyncWithDisk();
    index.RegisterMap(mwm);
  }

  TestSearchEngine engine(index, move(infoGetter), Engine::Params{});

  vector<Stats> stats(samples.size());
  FeatureLoader loader(index);
  Matcher matcher(loader);

  cout << "SampleId,";
  RankingInfo::PrintCSVHeader(cout);
  cout << ",Relevance" << endl;

  for (size_t i = 0; i < samples.size(); ++i)
  {
    auto const & sample = samples[i];

    search::SearchParams params;
    sample.FillSearchParams(params);
    TestSearchRequest request(engine, params);
    request.Run();

    auto const & results = request.Results();

    vector<size_t> goldenMatching;
    vector<size_t> actualMatching;
    matcher.Match(sample.m_results, results, goldenMatching, actualMatching);

    for (size_t j = 0; j < results.size(); ++j)
    {
      if (results[j].GetResultType() != Result::Type::Feature)
        continue;
      auto const & info = results[j].GetRankingInfo();
      cout << i << ",";
      info.ToCSV(cout);

      auto relevance = Sample::Result::Relevance::Irrelevant;
      if (actualMatching[j] != Matcher::kInvalidId)
        relevance = sample.m_results[actualMatching[j]].m_relevance;
      cout << "," << DebugPrint(relevance) << endl;
    }

    auto & s = stats[i];
    for (size_t j = 0; j < goldenMatching.size(); ++j)
    {
      if (goldenMatching[j] == Matcher::kInvalidId &&
          sample.m_results[j].m_relevance != Sample::Result::Relevance::Irrelevant)
      {
        s.m_notFound.push_back(j);
      }
    }
  }

  if (FLAGS_stats_path.empty())
  {
    cerr << string(34, '=') << " Statistics " << string(34, '=') << endl;
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
