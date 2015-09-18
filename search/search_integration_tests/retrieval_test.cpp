#include "testing/testing.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/scales.hpp"
#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "search/retrieval.hpp"
#include "search/search_query_params.hpp"
#include "search/search_tests_support/test_feature.hpp"
#include "search/search_tests_support/test_mwm_builder.hpp"
#include "search/search_tests_support/test_search_engine.hpp"
#include "search/search_tests_support/test_search_request.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/initializer_list.hpp"
#include "std/sstream.hpp"
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

class MatchingRule
{
public:
  virtual ~MatchingRule() = default;

  virtual bool Matches(FeatureType const & feature) const = 0;
  virtual string ToString() const = 0;
};

string DebugPrint(MatchingRule const & rule) { return rule.ToString(); }

class ExactMatch : public MatchingRule
{
public:
  ExactMatch(MwmSet::MwmId const & mwmId, shared_ptr<TestFeature> feature)
    : m_mwmId(mwmId), m_feature(feature)
  {
  }

  // MatchingRule overrides:
  bool Matches(FeatureType const & feature) const override
  {
    if (m_mwmId != feature.GetID().m_mwmId)
      return false;
    return m_feature->Matches(feature);
  }

  string ToString() const override
  {
    ostringstream os;
    os << "ExactMatch [ " << DebugPrint(m_mwmId) << ", " << DebugPrint(*m_feature) << " ]";
    return os.str();
  }

private:
  MwmSet::MwmId m_mwmId;
  shared_ptr<TestFeature> m_feature;
};

class AlternativesMatch : public MatchingRule
{
public:
  AlternativesMatch(initializer_list<shared_ptr<MatchingRule>> rules) : m_rules(move(rules)) {}

  // MatchingRule overrides:
  bool Matches(FeatureType const & feature) const override
  {
    for (auto const & rule : m_rules)
    {
      if (rule->Matches(feature))
        return true;
    }
    return false;
  }

  string ToString() const override
  {
    ostringstream os;
    os << "OrRule [ ";
    for (auto it = m_rules.cbegin(); it != m_rules.cend(); ++it)
    {
      os << (*it)->ToString();
      if (it + 1 != m_rules.end())
        os << " | ";
    }
    os << " ]";
    return os.str();
  }

private:
  vector<shared_ptr<MatchingRule>> m_rules;
};

void MatchResults(Index const & index, vector<shared_ptr<MatchingRule>> rules,
                  vector<search::Result> const & actual)
{
  vector<FeatureID> resultIds;
  for (auto const & a : actual)
    resultIds.push_back(a.GetFeatureID());
  sort(resultIds.begin(), resultIds.end());

  vector<string> unexpected;
  auto removeMatched = [&rules, &unexpected](FeatureType const & feature)
  {
    for (auto it = rules.begin(); it != rules.end(); ++it)
    {
      if ((*it)->Matches(feature))
      {
        rules.erase(it);
        return;
      }
    }
    unexpected.push_back(DebugPrint(feature) + " from " + DebugPrint(feature.GetID().m_mwmId));
  };
  index.ReadFeatures(removeMatched, resultIds);

  if (rules.empty() && unexpected.empty())
    return;

  ostringstream os;
  os << "Non-satisfied rules:" << endl;
  for (auto const & e : rules)
    os << "  " << DebugPrint(*e) << endl;
  os << "Unexpected retrieved features:" << endl;
  for (auto const & u : unexpected)
    os << "  " << u << endl;

  TEST(false, (os.str()));
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
                           vector<uint32_t> const & offsets) override
  {
    TEST_EQUAL(m_id, id, ());
    m_triggered = true;
    m_offsets.insert(m_offsets.end(), offsets.begin(), offsets.end());
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
                           vector<uint32_t> const & offsets) override
  {
    auto const it = find(m_ids.cbegin(), m_ids.cend(), id);
    TEST(it != m_ids.cend(), ("Unknown mwm:", id));

    m_retrieved.insert(id);
    m_numFeatures += offsets.size();
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
  TEST_EQUAL(MapOptions::Map, file.GetFiles(), ());

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
  {
    TestCallback callback(id);
    search::Retrieval::Limits limits;
    limits.SetMaxViewportScale(9.0);

    retrieval.Init(index, infos, m2::RectD(m2::PointD(0, 0), m2::PointD(1, 1)), params, limits);
    retrieval.Go(callback);
    TEST(callback.WasTriggered(), ());
    TEST_EQUAL(36 /* number of whiskey bars in a 5 x 5 square (border is counted) */,
               callback.Offsets().size(), ());
  }

  // Retrieve exactly 8 whiskey bars from the center.
  {
    TestCallback callback(id);
    search::Retrieval::Limits limits;
    limits.SetMaxNumFeatures(8);

    retrieval.Init(index, infos, m2::RectD(m2::PointD(4.9, 4.9), m2::PointD(5.1, 5.1)), params,
                   limits);
    retrieval.Go(callback);
    TEST(callback.WasTriggered(), ());
    TEST_EQUAL(callback.Offsets().size(), 8, ());
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
    TEST(callback.WasTriggered(), ());
    TEST_EQUAL(callback.Offsets().size(), 1, ());
  }

  {
    MultiMwmCallback callback({mskId, mtvId, zrhId});
    search::Retrieval::Limits limits;
    limits.SetMaxNumFeatures(10 /* more than total number of features in all these mwms */);

    retrieval.Init(index, infos, m2::RectD(m2::PointD(-1.0, -1.0), m2::PointD(1.0, 1.0)), params,
                   limits);
    retrieval.Go(callback);
    TEST_EQUAL(3 /* total number of mwms */, callback.GetNumMwms(), ());
    TEST_EQUAL(3 /* total number of features in all these mwms */, callback.GetNumFeatures(), ());
  }

  {
    MultiMwmCallback callback({mskId, mtvId, zrhId});
    search::Retrieval::Limits limits;

    retrieval.Init(index, infos, m2::RectD(m2::PointD(-1.0, -1.0), m2::PointD(1.0, 1.0)), params,
                   limits);
    retrieval.Go(callback);
    TEST_EQUAL(3, callback.GetNumMwms(), ());
    TEST_EQUAL(3, callback.GetNumFeatures(), ());
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

  auto const moscowCity = make_shared<TestCity>(m2::PointD(1, 0), "Moscow", "en", 100 /* rank */);
  auto const mtvCafe = make_shared<TestPOI>(m2::PointD(1, 0), "Cafe MTV", "en");

  auto const mtvCity = make_shared<TestCity>(m2::PointD(-1, 0), "MTV", "en", 100 /* rank */);
  auto const moscowCafe = make_shared<TestPOI>(m2::PointD(-1, 0), "Cafe Moscow", "en");

  {
    TestMwmBuilder builder(msk, feature::DataHeader::country);
    builder.Add(*moscowCity);
    builder.Add(*mtvCafe);
  }
  {
    TestMwmBuilder builder(mtv, feature::DataHeader::country);
    builder.Add(*mtvCity);
    builder.Add(*moscowCafe);
  }
  {
    TestMwmBuilder builder(testWorld, feature::DataHeader::world);
    builder.Add(*moscowCity);
    builder.Add(*mtvCity);
  }

  TestSearchEngine engine("en");
  TEST_EQUAL(MwmSet::RegResult::Success, engine.RegisterMap(msk).second, ());
  TEST_EQUAL(MwmSet::RegResult::Success, engine.RegisterMap(mtv).second, ());
  TEST_EQUAL(MwmSet::RegResult::Success, engine.RegisterMap(testWorld).second, ());

  auto const mskId = engine.GetMwmIdByCountryFile(msk.GetCountryFile());
  auto const mtvId = engine.GetMwmIdByCountryFile(mtv.GetCountryFile());
  auto const testWorldId = engine.GetMwmIdByCountryFile(testWorld.GetCountryFile());

  m2::RectD const moscowViewport(m2::PointD(0.99, -0.1), m2::PointD(1.01, 0.1));
  m2::RectD const mtvViewport(m2::PointD(-1.1, -0.1), m2::PointD(-0.99, 0.1));

  {
    TestSearchRequest request(engine, "Moscow ", "en", search::SearchParams::ALL, moscowViewport);
    request.Wait();

    vector<shared_ptr<MatchingRule>> rules = {make_shared<ExactMatch>(testWorldId, moscowCity),
                                              make_shared<ExactMatch>(mtvId, moscowCafe)};
    MatchResults(engine, rules, request.Results());
  }

  {
    TestSearchRequest request(engine, "MTV ", "en", search::SearchParams::ALL, mtvViewport);
    request.Wait();
    vector<shared_ptr<MatchingRule>> rules = {make_shared<ExactMatch>(testWorldId, mtvCity),
                                              make_shared<ExactMatch>(mskId, mtvCafe)};
    MatchResults(engine, rules, request.Results());
  }

  {
    TestSearchRequest request(engine, "Moscow MTV ", "en", search::SearchParams::ALL, mtvViewport);
    request.Wait();

    initializer_list<shared_ptr<MatchingRule>> alternatives = {
        make_shared<ExactMatch>(mskId, mtvCafe), make_shared<ExactMatch>(mtvId, moscowCafe)};
    vector<shared_ptr<MatchingRule>> rules = {make_shared<AlternativesMatch>(alternatives)};

    // TODO (@gorshenin): current search algorithms can't retrieve
    // both Cafe Moscow @ MTV and Cafe MTV @ Moscow, it'll just return
    // one of them. Fix this test when locality search will be fixed.
    MatchResults(engine, rules, request.Results());
  }
}
