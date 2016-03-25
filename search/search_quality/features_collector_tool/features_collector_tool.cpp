#include "search/result.hpp"
#include "search/search_quality/helpers.hpp"
#include "search/search_quality/sample.hpp"
#include "search/search_tests_support/test_search_engine.hpp"
#include "search/search_tests_support/test_search_request.hpp"
#include "search/v2/ranking_info.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/feature_algo.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/index.hpp"
#include "storage/storage.hpp"

#include "coding/file_name_utils.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

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
DEFINE_string(json_in, "", "Path to the json file with samples (default: stdin)");

void DidDownload(TCountryId const & /* countryId */,
                 shared_ptr<platform::LocalCountryFile> const & /* localFile */)
{
}

bool WillDelete(TCountryId const & /* countryId */,
                shared_ptr<platform::LocalCountryFile> const & /* localFile */)
{
  return false;
}

struct Context
{
  Context(Index & index) : m_index(index) {}

  void GetFeature(FeatureID const & id, FeatureType & ft)
  {
    auto const & mwmId = id.m_mwmId;
    if (!m_guard || m_guard->GetId() != mwmId)
      m_guard = make_unique<Index::FeaturesLoaderGuard>(m_index, mwmId);
    m_guard->GetFeatureByIndex(id.m_index, ft);
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
  static double constexpr kEps = 2e-5;
  if (actual.GetResultType() != Result::RESULT_FEATURE)
    return false;

  FeatureType ft;
  context.GetFeature(actual.GetFeatureID(), ft);

  string name;
  if (!ft.GetName(FeatureType::DEFAULT_LANG, name))
    name.clear();
  auto const houseNumber = ft.GetHouseNumber();
  auto const center = feature::GetCenter(ft);

  return golden.m_name == strings::MakeUniString(name) && golden.m_houseNumber == houseNumber &&
         my::AlmostEqualAbs(golden.m_pos, center, kEps);
}

void SetRelevanceValues(Context & context, vector<Sample::Result> const & golden,
                        vector<search::Result> const & actual,
                        vector<Sample::Result::Relevance> & relevances)
{
  auto const n = golden.size();
  auto const m = actual.size();
  relevances.assign(m, Sample::Result::RELEVANCE_IRRELEVANT);

  vector<bool> matched(m);

  // TODO (@y, @m): use Kuhn algorithm here for maximum matching.
  for (size_t i = 0; i < n; ++i)
  {
    auto const & g = golden[i];
    for (size_t j = 0; j < m; ++j)
    {
      if (matched[j])
        continue;
      auto const & a = actual[j];
      if (Matches(context, g, a))
      {
        matched[j] = true;
        relevances[j] = g.m_relevance;
        break;
      }
    }
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
  TestSearchEngine engine(move(infoGetter), make_unique<SearchQueryFactory>(), Engine::Params{});

  vector<platform::LocalCountryFile> mwms;
  platform::FindAllLocalMapsAndCleanup(numeric_limits<int64_t>::max() /* the latest version */,
                                       mwms);
  for (auto & mwm : mwms)
  {
    mwm.SyncWithDisk();
    engine.RegisterMap(mwm);
  }

  cout << "SampleId,";
  v2::RankingInfo::PrintCSVHeader(cout);
  cout << ",Relevance" << endl;

  Context context(engine);
  for (size_t i = 0; i < samples.size(); ++i)
  {
    auto const & sample = samples[i];

    engine.SetLocale(sample.m_locale);

    auto latLon = MercatorBounds::ToLatLon(sample.m_pos);

    search::SearchParams params;
    params.m_query = strings::ToUtf8(sample.m_query);
    params.m_inputLocale = sample.m_locale;
    params.SetMode(Mode::Everywhere);
    params.SetPosition(latLon.lat, latLon.lon);
    params.SetSuggestsEnabled(false);
    TestSearchRequest request(engine, params, sample.m_viewport);
    request.Wait();

    auto const & results = request.Results();

    vector<Sample::Result::Relevance> relevances;
    SetRelevanceValues(context, sample.m_results, results, relevances);

    ASSERT_EQUAL(results.size(), relevances.size(), ());
    for (size_t j = 0; j < results.size(); ++j)
    {
      if (results[j].GetResultType() != Result::RESULT_FEATURE)
        continue;
      auto const & info = results[j].GetRankingInfo();
      cout << i << ",";
      info.ToCSV(cout);
      cout << "," << DebugPrint(relevances[j]) << endl;
    }
  }
  return 0;
}
