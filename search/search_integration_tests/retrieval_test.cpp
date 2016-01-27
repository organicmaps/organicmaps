#include "testing/testing.hpp"

#include "search/retrieval.hpp"
#include "search/search_delimiters.hpp"
#include "search/search_query_params.hpp"
#include "search/search_string_utils.hpp"
#include "search/search_tests_support/test_feature.hpp"
#include "search/search_tests_support/test_mwm_builder.hpp"
#include "search/search_tests_support/test_results_matching.hpp"
#include "search/search_tests_support/test_search_engine.hpp"
#include "search/search_tests_support/test_search_request.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/scales.hpp"

#include "storage/country_decl.hpp"
#include "storage/country_info_getter.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "coding/compressed_bit_vector.hpp"

#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/initializer_list.hpp"
#include "std/limits.hpp"
#include "std/shared_ptr.hpp"

using namespace search::tests_support;

namespace
{
void InitParams(string const & query, search::SearchQueryParams & params)
{
  params.Clear();
  auto insertTokens = [&params](strings::UniString const & token)
  {
    params.m_tokens.push_back({token});
  };
  search::Delimiters delims;
  SplitUniString(search::NormalizeAndSimplifyString(query), insertTokens, delims);

  params.m_langs.insert(StringUtf8Multilang::GetLangIndex("en"));
}

bool AllMwmsReleased(Index & index)
{
  vector<shared_ptr<MwmInfo>> infos;
  index.GetMwmsInfo(infos);
  for (auto const & info : infos)
  {
    if (info->GetNumRefs() != 0)
      return false;
  }
  return true;
}

void Cleanup(platform::LocalCountryFile const & map)
{
  platform::CountryIndexes::DeleteFromDisk(map);
  map.DeleteFromDisk(MapOptions::Map);
}

void Cleanup(initializer_list<platform::LocalCountryFile> const & maps)
{
  for (auto const & map : maps)
    Cleanup(map);
}

class TestCallback : public search::Retrieval::Callback
{
public:
  TestCallback(MwmSet::MwmId const & id) : m_id(id), m_triggered(false) {}

  // search::Retrieval::Callback overrides:
  void OnFeaturesRetrieved(MwmSet::MwmId const & id, double scale,
                           coding::CompressedBitVector const & features) override
  {
    TEST_EQUAL(m_id, id, ());
    m_triggered = true;
    coding::CompressedBitVectorEnumerator::ForEach(
        features, [&](uint64_t featureId)
        {
          CHECK_LESS(featureId, numeric_limits<uint32_t>::max(), ());
          m_offsets.push_back(static_cast<uint32_t>(featureId));
        });
  }

  void OnMwmProcessed(MwmSet::MwmId const & /* id */) override {}

  bool WasTriggered() const { return m_triggered; }

  vector<uint32_t> & Offsets() { return m_offsets; }
  vector<uint32_t> const & Offsets() const { return m_offsets; }

private:
  MwmSet::MwmId const m_id;
  vector<uint32_t> m_offsets;
  bool m_triggered;
};

class MultiMwmCallback : public search::Retrieval::Callback
{
public:
  MultiMwmCallback(vector<MwmSet::MwmId> const & ids) : m_ids(ids), m_numFeatures(0) {}

  // search::Retrieval::Callback overrides:
  void OnFeaturesRetrieved(MwmSet::MwmId const & id, double /* scale */,
                           coding::CompressedBitVector const & features) override
  {
    auto const it = find(m_ids.cbegin(), m_ids.cend(), id);
    TEST(it != m_ids.cend(), ("Unknown mwm:", id));

    m_retrieved.insert(id);
    m_numFeatures += features.PopCount();
  }

  void OnMwmProcessed(MwmSet::MwmId const & /* id */) override {}

  uint64_t GetNumMwms() const { return m_retrieved.size(); }

  uint64_t GetNumFeatures() const { return m_numFeatures; }

private:
  vector<MwmSet::MwmId> m_ids;
  set<MwmSet::MwmId> m_retrieved;
  uint64_t m_numFeatures;
};
}  // namespace

UNIT_TEST(Retrieval_Smoke)
{
  classificator::Load();
  Platform & platform = GetPlatform();

  platform::LocalCountryFile file(platform.WritableDir(), platform::CountryFile("WhiskeyTown"), 0);
  Cleanup(file);
  MY_SCOPE_GUARD(deleteFile, [&]()
  {
    Cleanup(file);
  });

  // Create a test mwm with 100 whiskey bars.
  {
    TestMwmBuilder builder(file, feature::DataHeader::country);
    for (int x = 0; x < 10; ++x)
    {
      for (int y = 0; y < 10; ++y)
        builder.Add(TestPOI(m2::PointD(x, y), "Whiskey bar", "en"));
    }
  }
  TEST_EQUAL(MapOptions::MapWithCarRouting, file.GetFiles(), ());

  Index index;
  auto p = index.RegisterMap(file);
  auto & id = p.first;
  TEST(id.IsAlive(), ());
  TEST_EQUAL(p.second, MwmSet::RegResult::Success, ());

  search::SearchQueryParams params;
  InitParams("whiskey bar", params);

  search::Retrieval retrieval;

  vector<shared_ptr<MwmInfo>> infos;
  index.GetMwmsInfo(infos);

  // Retrieve all (100) whiskey bars from the mwm.
  {
    TestCallback callback(id);

    retrieval.Init(index, infos, m2::RectD(m2::PointD(0, 0), m2::PointD(1, 1)), params,
                   search::Retrieval::Limits());
    retrieval.Go(callback);
    TEST(callback.WasTriggered(), ());
    TEST_EQUAL(100, callback.Offsets().size(), ());

    TestCallback dummyCallback(id);
    retrieval.Go(dummyCallback);
    TEST(!dummyCallback.WasTriggered(), ());
  }

  // Retrieve all whiskey bars from the left-bottom 5 x 5 square.
  // Note that due to current coverage algorithm the number of
  // retrieved results can be greater than 36.
  {
    TestCallback callback(id);
    search::Retrieval::Limits limits;
    limits.SetMaxViewportScale(9.0);

    retrieval.Init(index, infos, m2::RectD(m2::PointD(0, 0), m2::PointD(1, 1)), params, limits);
    retrieval.Go(callback);
    TEST(callback.WasTriggered(), ());
    TEST_GREATER_OR_EQUAL(callback.Offsets().size(),
                          36 /* number of whiskey bars in a 5 x 5 square (border is counted) */,
                          ());
  }

  // Retrieve exactly 8 whiskey bars from the center.  Note that due
  // to current coverage algorithm the number of retrieved results can
  // be greater than 8.
  {
    TestCallback callback(id);
    search::Retrieval::Limits limits;
    limits.SetMaxNumFeatures(8);

    retrieval.Init(index, infos, m2::RectD(m2::PointD(4.9, 4.9), m2::PointD(5.1, 5.1)), params,
                   limits);
    retrieval.Go(callback);
    TEST(callback.WasTriggered(), ());
    TEST_GREATER_OR_EQUAL(callback.Offsets().size(), 8, ());
  }
}

UNIT_TEST(Retrieval_3Mwms)
{
  classificator::Load();
  Platform & platform = GetPlatform();

  platform::LocalCountryFile msk(platform.WritableDir(), platform::CountryFile("msk"), 0);
  platform::LocalCountryFile mtv(platform.WritableDir(), platform::CountryFile("mtv"), 0);
  platform::LocalCountryFile zrh(platform.WritableDir(), platform::CountryFile("zrh"), 0);
  Cleanup({msk, mtv, zrh});
  MY_SCOPE_GUARD(cleanup, [&]()
  {
    Cleanup({msk, mtv, zrh});
  });

  {
    TestMwmBuilder builder(msk, feature::DataHeader::country);
    builder.Add(TestPOI(m2::PointD(0, 0), "Cafe MTV", "en"));
  }
  {
    TestMwmBuilder builder(mtv, feature::DataHeader::country);
    builder.Add(TestPOI(m2::PointD(10, 0), "MTV", "en"));
  }
  {
    TestMwmBuilder builder(zrh, feature::DataHeader::country);
    builder.Add(TestPOI(m2::PointD(0, 10), "Bar MTV", "en"));
  }

  Index index;
  auto mskP = index.RegisterMap(msk);
  auto & mskId = mskP.first;

  auto mtvP = index.RegisterMap(mtv);
  auto & mtvId = mtvP.first;

  auto zrhP = index.RegisterMap(zrh);
  auto & zrhId = zrhP.first;

  TEST(mskId.IsAlive(), ());
  TEST(mtvId.IsAlive(), ());
  TEST(zrhId.IsAlive(), ());

  search::SearchQueryParams params;
  InitParams("mtv", params);

  vector<shared_ptr<MwmInfo>> infos;
  index.GetMwmsInfo(infos);

  search::Retrieval retrieval;

  {
    TestCallback callback(mskId);
    search::Retrieval::Limits limits;
    limits.SetMaxNumFeatures(1);

    retrieval.Init(index, infos, m2::RectD(m2::PointD(-1.0, -1.0), m2::PointD(1.0, 1.0)), params,
                   limits);
    retrieval.Go(callback);
    retrieval.Release();

    TEST(callback.WasTriggered(), ());
    TEST_EQUAL(callback.Offsets().size(), 1, ());
    TEST(AllMwmsReleased(index), ());
  }

  {
    MultiMwmCallback callback({mskId, mtvId, zrhId});
    search::Retrieval::Limits limits;
    limits.SetMaxNumFeatures(10 /* more than total number of features in all these mwms */);

    retrieval.Init(index, infos, m2::RectD(m2::PointD(-1.0, -1.0), m2::PointD(1.0, 1.0)), params,
                   limits);
    retrieval.Go(callback);
    retrieval.Release();

    TEST_EQUAL(3 /* total number of mwms */, callback.GetNumMwms(), ());
    TEST_EQUAL(3 /* total number of features in all these mwms */, callback.GetNumFeatures(), ());
    TEST(AllMwmsReleased(index), ());
  }

  {
    MultiMwmCallback callback({mskId, mtvId, zrhId});
    search::Retrieval::Limits limits;

    retrieval.Init(index, infos, m2::RectD(m2::PointD(-1.0, -1.0), m2::PointD(1.0, 1.0)), params,
                   limits);
    retrieval.Go(callback);
    retrieval.Release();

    TEST_EQUAL(3, callback.GetNumMwms(), ());
    TEST_EQUAL(3, callback.GetNumFeatures(), ());
    TEST(AllMwmsReleased(index), ());
  }
}

// This test creates a couple of maps:
// * model map of Moscow, with "Cafe MTV" at the center
// * model map of MTV, with "Cafe Moscow" at the center
// * TestWorld map, with Moscow and MTV
// TestWorld map is needed because locality search works only on world maps.
//
// Then, it tests search engine on following requests:
//
// 1. "Moscow", viewport covers only Moscow - should return two
// results (Moscow city and Cafe Moscow at MTV).
// 2. "MTV", viewport covers only MTV"- should return two results (MTV
// city and Cafe MTV at Moscow).
// 3. "Moscow MTV", viewport covers only MTV - should return MTV cafe
// at Moscow.
// 4. "MTV Moscow", viewport covers only Moscow - should return Moscow
// cafe at MTV.
UNIT_TEST(Retrieval_CafeMTV)
{
  classificator::Load();
  Platform & platform = GetPlatform();

  platform::LocalCountryFile msk(platform.WritableDir(), platform::CountryFile("msk"), 0);
  platform::LocalCountryFile mtv(platform.WritableDir(), platform::CountryFile("mtv"), 0);
  platform::LocalCountryFile testWorld(platform.WritableDir(), platform::CountryFile("testWorld"),
                                       0);
  Cleanup({msk, mtv, testWorld});
  MY_SCOPE_GUARD(cleanup, [&]()
  {
    Cleanup({msk, mtv, testWorld});
  });

  auto const mskCity = make_shared<TestCity>(m2::PointD(1, 0), "Moscow", "en", 100 /* rank */);
  auto const mtvCafe = make_shared<TestPOI>(m2::PointD(1, 0), "Cafe MTV", "en");

  auto const mtvCity = make_shared<TestCity>(m2::PointD(-1, 0), "MTV", "en", 100 /* rank */);
  auto const mskCafe = make_shared<TestPOI>(m2::PointD(-1, 0), "Cafe Moscow", "en");

  {
    TestMwmBuilder builder(msk, feature::DataHeader::country);
    builder.Add(*mskCity);
    builder.Add(*mtvCafe);
  }
  {
    TestMwmBuilder builder(mtv, feature::DataHeader::country);
    builder.Add(*mtvCity);
    builder.Add(*mskCafe);
  }
  {
    TestMwmBuilder builder(testWorld, feature::DataHeader::world);
    builder.Add(*mskCity);
    builder.Add(*mtvCity);
  }

  m2::RectD const mskViewport(m2::PointD(0.99, -0.1), m2::PointD(1.01, 0.1));
  m2::RectD const mtvViewport(m2::PointD(-1.1, -0.1), m2::PointD(-0.99, 0.1));

  // There are test requests involving locality search, thus it's
  // better to mock information about countries.
  vector<storage::CountryDef> countries;
  countries.emplace_back(msk.GetCountryName(), mskViewport);
  countries.emplace_back(mtv.GetCountryName(), mtvViewport);

  TestSearchEngine engine("en", make_unique<storage::CountryInfoGetterForTesting>(countries));
  TEST_EQUAL(MwmSet::RegResult::Success, engine.RegisterMap(msk).second, ());
  TEST_EQUAL(MwmSet::RegResult::Success, engine.RegisterMap(mtv).second, ());
  TEST_EQUAL(MwmSet::RegResult::Success, engine.RegisterMap(testWorld).second, ());

  auto const mskId = engine.GetMwmIdByCountryFile(msk.GetCountryFile());
  auto const mtvId = engine.GetMwmIdByCountryFile(mtv.GetCountryFile());
  auto const testWorldId = engine.GetMwmIdByCountryFile(testWorld.GetCountryFile());

  {
    TestSearchRequest request(engine, "Moscow ", "en", search::SearchParams::ALL, mskViewport);
    request.Wait();

    initializer_list<shared_ptr<MatchingRule>> mskCityAlts = {
        make_shared<ExactMatch>(testWorldId, mskCity), make_shared<ExactMatch>(mskId, mskCity)};
    vector<shared_ptr<MatchingRule>> rules = {make_shared<AlternativesMatch>(mskCityAlts),
                                              make_shared<ExactMatch>(mtvId, mskCafe)};
    TEST(MatchResults(engine, rules, request.Results()), ());
  }

  {
    TestSearchRequest request(engine, "MTV ", "en", search::SearchParams::ALL, mtvViewport);
    request.Wait();

    initializer_list<shared_ptr<MatchingRule>> mtvCityAlts = {
        make_shared<ExactMatch>(testWorldId, mtvCity), make_shared<ExactMatch>(mtvId, mtvCity)};
    vector<shared_ptr<MatchingRule>> rules = {make_shared<AlternativesMatch>(mtvCityAlts),
                                              make_shared<ExactMatch>(mskId, mtvCafe)};
    TEST(MatchResults(engine, rules, request.Results()), ());
  }

  {
    TestSearchRequest request(engine, "Moscow MTV ", "en", search::SearchParams::ALL, mtvViewport);
    request.Wait();

    initializer_list<shared_ptr<MatchingRule>> alternatives = {
        make_shared<ExactMatch>(mskId, mtvCafe), make_shared<ExactMatch>(mtvId, mskCafe)};
    vector<shared_ptr<MatchingRule>> rules = {make_shared<AlternativesMatch>(alternatives)};

    // TODO (@gorshenin): current search algorithm can't retrieve both
    // Cafe Moscow @ MTV and Cafe MTV @ Moscow, it'll just return one
    // of them. Fix this test when locality search will be fixed.
    TEST(MatchResults(engine, rules, request.Results()), ());
  }
}
