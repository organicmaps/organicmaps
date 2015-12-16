#include "search/params.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "storage/country_info_getter.hpp"

#include "geometry/point2d.hpp"

#include "search/search_tests_support/test_search_engine.hpp"
#include "search/search_tests_support/test_search_request.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include "std/cstdio.hpp"
#include "std/numeric.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

#include "3party/gflags/src/gflags/gflags.h"

using namespace search::tests_support;

#pragma mark Define options
DEFINE_string(data_path, "", "Path to data directory (resources dir)");
DEFINE_string(locale, "en", "Locale of all the search queries");
DEFINE_string(mwm_list_path, "", "Path to a file containing the names of available mwms, one per line");
DEFINE_string(mwm_path, "", "Path to mwm files (writable dir)");
DEFINE_string(queries_path, "", "Path to the file with queries");
DEFINE_int32(top, 1, "Number of top results to show for every query");

size_t const kNumTopResults = 5;
string const kDefaultQueriesPathSuffix = "/../search/search_quality_tests/queries.txt";

class SearchQueryV2Factory : public search::SearchQueryFactory
{
  // search::SearchQueryFactory overrides:
  unique_ptr<search::Query> BuildSearchQuery(Index & index, CategoriesHolder const & categories,
                                             vector<search::Suggest> const & suggests,
                                             storage::CountryInfoGetter const & infoGetter) override
  {
    return make_unique<search::v2::SearchQueryV2>(index, categories, suggests, infoGetter);
  }
};

unique_ptr<storage::CountryInfoGetter> CreateCountryInfoGetter()
{
  Platform & platform = GetPlatform();
  return make_unique<storage::CountryInfoReader>(platform.GetReader(PACKED_POLYGONS_FILE),
                                                 platform.GetReader(COUNTRIES_FILE));
}

void ReadStringsFromFile(string const & path, vector<string> & result)
{
  FILE * f = fopen(path.data(), "r");
  CHECK(f != nullptr, ("Error when reading strings from", path, ":", string(strerror(errno))));
  MY_SCOPE_GUARD(cleanup, [&]
                 {
                   fclose(f);
                 });

  int const kBufSize = 1 << 20;
  char buf[kBufSize];
  while (fgets(buf, sizeof(buf), f))
  {
    string s(buf);
    strings::Trim(s);
    if (!s.empty())
      result.push_back(s);
  }
}

void PrintTopResults(string const & query, vector<search::Result> const & results, size_t n)
{
  printf("%s", query.data());
  for (size_t i = 0; i < n; i++)
  {
    if (n > 1)
      printf("\n");
    printf("\t");
    if (i < results.size())
      // todo(@m) Print more information: coordinates, viewport, etc.
      printf("%s", results[i].GetString());
    else
      printf("<empty>");
  }
  printf("\n");
}

int main(int argc, char * argv[])
{
  Platform & platform = GetPlatform();

  google::SetUsageMessage("Search quality tests.");
  google::ParseCommandLineFlags(&argc, &argv, true);

  if (!FLAGS_data_path.empty())
    platform.SetResourceDir(FLAGS_data_path);

  if (!FLAGS_mwm_path.empty())
    platform.SetWritableDirForTests(FLAGS_mwm_path);

  LOG(LINFO, ("writable dir =", platform.WritableDir()));
  LOG(LINFO, ("resources dir =", platform.ResourcesDir()));

  classificator::Load();
  auto infoGetter = CreateCountryInfoGetter();

  TestSearchEngine engine(FLAGS_locale, move(infoGetter), make_unique<SearchQueryV2Factory>());

  vector<platform::LocalCountryFile> mwms;
  if (!FLAGS_mwm_list_path.empty())
  {
    vector<string> availableMwms;
    ReadStringsFromFile(FLAGS_mwm_list_path, availableMwms);
    for (auto const & countryName : availableMwms)
      mwms.emplace_back(platform.WritableDir(), platform::CountryFile(countryName), 0);
  }
  else
  {
    platform::FindAllLocalMapsAndCleanup(numeric_limits<int64_t>::max() /* the latest version */,
                                         mwms);
    for (auto & map : mwms)
      map.SyncWithDisk();
  }
  for (auto const & mwm : mwms)
  {
    LOG(LINFO, ("Registering map:", mwm.GetCountryName()));
    engine.RegisterMap(mwm);
  }

  vector<string> queries;
  string queriesPath = FLAGS_queries_path;
  if (queriesPath.empty())
    queriesPath = platform.WritableDir() + kDefaultQueriesPathSuffix;
  ReadStringsFromFile(queriesPath, queries);

  for (string const & query : queries)
  {
    // todo(@m) Viewport and position should belong to the query info.
    m2::RectD viewport(m2::PointD(0.0, 0.0), m2::PointD(1.0, 1.0));
    TestSearchRequest request(engine, query, FLAGS_locale, search::SearchParams::ALL, viewport);
    request.Wait();

    PrintTopResults(query, request.Results(), FLAGS_top);
  }

  return 0;
}
