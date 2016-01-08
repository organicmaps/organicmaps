#include "search/params.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_header.hpp"
#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "storage/country_info_getter.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "search/search_tests_support/test_search_engine.hpp"
#include "search/search_tests_support/test_search_request.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/reader_wrapper.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/timer.hpp"

#include "std/cstdio.hpp"
#include "std/fstream.hpp"
#include "std/iostream.hpp"
#include "std/map.hpp"
#include "std/numeric.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

#include "defines.hpp"

#include "3party/gflags/src/gflags/gflags.h"

using namespace search::tests_support;

DEFINE_string(data_path, "", "Path to data directory (resources dir)");
DEFINE_string(locale, "en", "Locale of all the search queries");
DEFINE_string(mwm_list_path, "", "Path to a file containing the names of available mwms, one per line");
DEFINE_string(mwm_path, "", "Path to mwm files (writable dir)");
DEFINE_string(queries_path, "", "Path to the file with queries");
DEFINE_int32(top, 1, "Number of top results to show for every query");
DEFINE_string(viewport, "", "Viewport to use when searching (default, moscow, london, zurich)");

map<string, m2::RectD> const kViewports = {
    {"default", m2::RectD(m2::PointD(0.0, 0.0), m2::PointD(1.0, 1.0))},
    {"moscow", MercatorBounds::RectByCenterXYAndSizeInMeters(m2::PointD(37.7, 55.7), 5000)},
    {"london", MercatorBounds::RectByCenterXYAndSizeInMeters(m2::PointD(0.0, 51.5), 5000)},
    {"zurich", MercatorBounds::RectByCenterXYAndSizeInMeters(m2::PointD(8.5, 47.4), 5000)}};

string const kDefaultQueriesPathSuffix = "/../search/search_quality_tests/queries.txt";
string const kEmptyResult = "<empty>";

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

void ReadStringsFromFile(string const & path, vector<string> & result)
{
  ifstream stream(path.c_str());
  CHECK(stream.is_open(), ("Can't open", path));

  string s;
  while (getline(stream, s))
  {
    strings::Trim(s);
    if (!s.empty())
      result.emplace_back(s);
  }
}

// If n == 1, prints the query and the top result separated by a tab.
// Otherwise, prints the query on a separate line
// and then prints n top results on n lines starting with tabs.
void PrintTopResults(string const & query, vector<search::Result> const & results, size_t n,
                     double elapsedSeconds)
{
  cout << query;
  char timeBuf[100];
  snprintf(timeBuf, sizeof(timeBuf), "\t[%.3fs]", elapsedSeconds);
  if (n > 1)
    cout << timeBuf;
  for (size_t i = 0; i < n; ++i)
  {
    if (n > 1)
      cout << endl;
    cout << "\t";
    if (i < results.size())
      // todo(@m) Print more information: coordinates, viewport, etc.
      cout << results[i].GetString();
    else
      cout << kEmptyResult;
  }
  if (n == 1)
    cout << timeBuf;
  cout << endl;
}

uint64_t ReadVersionFromHeader(platform::LocalCountryFile const & mwm)
{
  if (mwm.GetCountryName() == WORLD_FILE_NAME || mwm.GetCountryName() == WORLD_COASTS_FILE_NAME)
    return mwm.GetVersion();

  ModelReaderPtr reader = FilesContainerR(mwm.GetPath(MapOptions::Map)).GetReader(VERSION_FILE_TAG);
  ReaderSrc src(reader.GetPtr());

  version::MwmVersion version;
  version::ReadVersion(src, version);
  return version.timestamp;
}

int main(int argc, char * argv[])
{
  ios_base::sync_with_stdio(false);
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

  TestSearchEngine engine(FLAGS_locale, make_unique<SearchQueryV2Factory>());

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
  cout << "Mwms used in all search invocations:" << endl;
  for (auto & mwm : mwms)
  {
    mwm.SyncWithDisk();
    cout << mwm.GetCountryName() << " " << ReadVersionFromHeader(mwm) << endl;
    engine.RegisterMap(mwm);
  }
  cout << endl;

  m2::RectD viewport;
  {
    string name = FLAGS_viewport;
    auto it = kViewports.find(name);
    if (it == kViewports.end())
    {
      name = "default";
      it = kViewports.find(name);
    }
    CHECK(it != kViewports.end(), ());
    viewport = it->second;
    cout << "Viewport used in all search invocations: " << name << " " << DebugPrint(viewport)
         << "\n\n";
  }

  vector<string> queries;
  string queriesPath = FLAGS_queries_path;
  if (queriesPath.empty())
    queriesPath = my::JoinFoldersToPath(platform.WritableDir(), kDefaultQueriesPathSuffix);
  ReadStringsFromFile(queriesPath, queries);

  for (string const & query : queries)
  {
    my::Timer timer;
    // todo(@m) Viewport and position should belong to the query info.
    TestSearchRequest request(engine, query, FLAGS_locale, search::SearchParams::ALL, viewport);
    request.Wait();

    PrintTopResults(query, request.Results(), FLAGS_top, timer.ElapsedSeconds());
  }

  return 0;
}
