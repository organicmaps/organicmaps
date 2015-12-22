#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "search/search_tests_support/test_search_engine.hpp"
#include "search/search_tests_support/test_search_request.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/scales.hpp"

#include "platform/country_defines.hpp"
#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

using namespace search::tests_support;

namespace
{
class ScopedMapFile
{
public:
  explicit ScopedMapFile(string const & name)
      : m_file(GetPlatform().TmpDir(), platform::CountryFile(name), 0)
  {
    platform::CountryIndexes::DeleteFromDisk(m_file);
  }

  ~ScopedMapFile()
  {
    platform::CountryIndexes::DeleteFromDisk(m_file);
    m_file.DeleteFromDisk(MapOptions::Map);
  }

  inline platform::LocalCountryFile & GetFile() { return m_file; }

private:
  platform::LocalCountryFile m_file;
};
}  // namespace

namespace search
{
void TestFeaturesCount(TestSearchEngine const & engine, m2::RectD const & rect,
                       size_t expectedCount)
{
  size_t actualCount = 0;
  auto counter = [&actualCount](const FeatureType & /* ft */)
  {
    ++actualCount;
  };
  engine.ForEachInRect(counter, rect, scales::GetUpperScale());
  TEST_EQUAL(expectedCount, actualCount, ());
}

UNIT_TEST(GenerateTestMwm_Smoke)
{
  classificator::Load();
  ScopedMapFile scopedFile("BuzzTown");
  platform::LocalCountryFile & file = scopedFile.GetFile();

  {
    TestMwmBuilder builder(file);
    builder.AddPOI(m2::PointD(0, 0), "Wine shop", "en");
    builder.AddPOI(m2::PointD(1, 0), "Tequila shop", "en");
    builder.AddPOI(m2::PointD(0, 1), "Brandy shop", "en");
    builder.AddPOI(m2::PointD(1, 1), "Russian vodka shop", "en");
  }
  TEST_EQUAL(MapOptions::Map, file.GetFiles(), ());

  TestSearchEngine engine("en" /* locale */);
  auto ret = engine.RegisterMap(file);
  TEST_EQUAL(MwmSet::RegResult::Success, ret.second, ("Can't register generated map."));
  TEST(ret.first.IsAlive(), ("Can't get lock on a generated map."));

  TestFeaturesCount(engine, m2::RectD(m2::PointD(0, 0), m2::PointD(1, 1)), 4);
  TestFeaturesCount(engine, m2::RectD(m2::PointD(-0.5, -0.5), m2::PointD(0.5, 1.5)), 2);

  {
    TestSearchRequest request(engine, "wine ", "en",
                              m2::RectD(m2::PointD(0, 0), m2::PointD(100, 100)));
    request.Wait();
    TEST_EQUAL(1, request.Results().size(), ());
  }

  {
    TestSearchRequest request(engine, "shop ", "en",
                              m2::RectD(m2::PointD(0, 0), m2::PointD(100, 100)));
    request.Wait();
    TEST_EQUAL(4, request.Results().size(), ());
  }
}

UNIT_TEST(GenerateTestMwm_NotPrefixFreeNames)
{
  classificator::Load();
  ScopedMapFile scopedFile("ATown");
  platform::LocalCountryFile & file = scopedFile.GetFile();

  {
    TestMwmBuilder builder(file);
    builder.AddPOI(m2::PointD(0, 0), "a", "en");
    builder.AddPOI(m2::PointD(0, 1), "aa", "en");
    builder.AddPOI(m2::PointD(1, 1), "aa", "en");
    builder.AddPOI(m2::PointD(1, 0), "aaa", "en");
    builder.AddPOI(m2::PointD(2, 0), "aaa", "en");
    builder.AddPOI(m2::PointD(2, 1), "aaa", "en");
  }
  TEST_EQUAL(MapOptions::Map, file.GetFiles(), ());

  TestSearchEngine engine("en" /* locale */);
  auto ret = engine.RegisterMap(file);
  TEST_EQUAL(MwmSet::RegResult::Success, ret.second, ("Can't register generated map."));
  TEST(ret.first.IsAlive(), ("Can't get lock on a generated map."));

  TestFeaturesCount(engine, m2::RectD(m2::PointD(0, 0), m2::PointD(2, 2)), 6);

  {
    TestSearchRequest request(engine, "a ", "en",
                              m2::RectD(m2::PointD(0, 0), m2::PointD(100, 100)));
    request.Wait();
    TEST_EQUAL(1, request.Results().size(), ());
  }
  {
    TestSearchRequest request(engine, "aa ", "en",
                              m2::RectD(m2::PointD(0, 0), m2::PointD(100, 100)));
    request.Wait();
    TEST_EQUAL(2, request.Results().size(), ());
  }
  {
    TestSearchRequest request(engine, "aaa ", "en",
                              m2::RectD(m2::PointD(0, 0), m2::PointD(100, 100)));
    request.Wait();
    TEST_EQUAL(3, request.Results().size(), ());
  }
}
}  // namespace search
