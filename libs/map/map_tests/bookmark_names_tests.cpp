#include "testing/testing.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/bookmarks_search_params.hpp"
#include "map/framework.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "storage/country_info_getter.hpp"

#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_utils.hpp"

#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

#include "coding/file_reader.hpp"
#include "coding/string_utf8_multilang.hpp"
#include "coding/transliteration.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"

#include <chrono>
#include <future>

namespace bookmark_names_tests
{
namespace
{
class TestFramework final : public Framework
{
public:
  TestFramework() : Framework({}, false /* loadMaps */) {}

  using Framework::OnCountryFileDownloaded;

  void RunUITask(std::function<void()> fn) override { fn(); }
};

class Fixture : public df::test_support::VisualParamsFixture
{
public:
  Fixture()
  {
    m_hadLanguage = settings::Get(settings::kMapLanguageCode, m_language);
    m_settingsDir = GetPlatform().SettingsDir();
    m_testDir = base::JoinPath(GetPlatform().WritableDir(), "bookmark_names_tests");
    TEST(Platform::MkDirRecursively(m_testDir), ());
    GetPlatform().SetSettingsDir(m_testDir);
    m_framework = std::make_unique<TestFramework>();
    m_transliteration = m_framework->LoadTransliteration();
    m_framework->GetBookmarkManager().EnableTestMode(true);
  }

  ~Fixture()
  {
    m_framework.reset();
    if (m_hadLanguage)
      settings::Set(settings::kMapLanguageCode, m_language);
    else
      settings::Delete(settings::kMapLanguageCode);
    Transliteration::Instance().SetMode(m_transliteration ? Transliteration::Mode::Enabled
                                                          : Transliteration::Mode::Disabled);
    GetPlatform().SetSettingsDir(m_settingsDir);
    Platform::RmDirRecursively(m_testDir);
  }

  TestFramework & GetFramework() { return *m_framework; }

private:
  std::string m_language, m_settingsDir, m_testDir;
  bool m_hadLanguage = false;
  bool m_transliteration = true;
  std::unique_ptr<TestFramework> m_framework;
};

kml::BookmarkData MakeBookmark(std::initializer_list<std::pair<std::string_view, std::string_view>> names,
                               std::vector<std::string> const & languages = {"zh"})
{
  kml::BookmarkData data;
  for (auto const & [code, value] : names)
    data.m_name[StringUtf8Multilang::GetLangIndex(code)] = value;
  data.m_featureTypes = {classif().GetTypeByPath({"amenity", "university"})};
  data.m_point = mercator::FromLatLon(24.437, 118.102);
  feature::RegionData region;
  region.SetLanguages(languages);
  kml::SetBookmarkRegionData(data, region);
  return data;
}

}  // namespace

UNIT_CLASS_TEST(Fixture, BookmarkNames_BackfillDoesNotRewriteFiles)
{
  GetFramework().SetMapLanguageCode("it");
  bool mapAvailable = false;
  size_t lookups = 0, notifications = 0;
  kml::MarkIdCollection updated;
  BookmarkManager::Callbacks callbacks(
      []() -> StringsBundle const &
  {
    static StringsBundle const bundle;
    return bundle;
  }, BookmarkManager::Callbacks::GetSeacrhAPIFn{}, BookmarkManager::Callbacks::CreatedBookmarksCallback{},
      [&](std::vector<BookmarkInfo> const & marks)
  {
    for (auto const & mark : marks)
      updated.push_back(mark.m_bookmarkId);
  }, BookmarkManager::Callbacks::DeletedBookmarksCallback{}, BookmarkManager::Callbacks::AttachedBookmarksCallback{},
      BookmarkManager::Callbacks::DetachedBookmarksCallback{}, [&](m2::PointD const &)
  {
    ++lookups;
    feature::RegionData region;
    if (mapAvailable)
      region.SetLanguages({"zh"});
    return region;
  });
  BookmarkManager manager(std::move(callbacks));
  manager.EnableTestMode(true);
  auto const category = manager.CreateBookmarkCategory("Legacy", true /* autoSave */);
  auto data = MakeBookmark({{"default", "厦门大学"}, {"en", "Xiamen University"}});
  data.m_properties.clear();
  auto elsewhereData = data;
  elsewhereData.m_point = mercator::FromLatLon(10, 10);
  auto const * bookmark = manager.GetEditSession().CreateBookmark(std::move(data), category);
  auto const * elsewhere = manager.GetEditSession().CreateBookmark(std::move(elsewhereData), category);
  manager.GetEditSession().CreateBookmark(MakeBookmark({{"default", "鼓东路"}}), category);
  TEST_EQUAL(lookups, 2, ());
  TEST_EQUAL(bookmark->GetPreferredName(), "厦门大学", ());
  auto const path = manager.GetCategoryFileName(category);
  std::string before;
  FileReader(path).ReadAsString(before);
  manager.SetBookmarksChangedCallback([&]() { ++notifications; });

  mapAvailable = true;
  auto const mapBounds = mercator::RectByCenterXYAndSizeInMeters(bookmark->GetPivot(), 1000);
  manager.UpdateBookmarkNames(mapBounds);
  TEST_EQUAL(bookmark->GetPreferredName(), "Xiamen University", ());
  TEST(kml::HasBookmarkRegionData(bookmark->GetData()), ());
  TEST(!kml::HasBookmarkRegionData(elsewhere->GetData()), ());
  TEST_EQUAL(updated, kml::MarkIdCollection{bookmark->GetId()}, ());
  TEST_EQUAL(notifications, 1, ());
  TEST_EQUAL(lookups, 3, ("Only the bookmark missing context inside the map needs a lookup"));
  std::string after;
  FileReader(path).ReadAsString(after);
  TEST_EQUAL(after, before, ());
  manager.UpdateBookmarkNames(mapBounds);
  TEST_EQUAL(lookups, 3, ("Saved context avoids repeated map lookups"));
  TEST_EQUAL(updated, kml::MarkIdCollection{bookmark->GetId()}, ());
  TEST_EQUAL(notifications, 1, ());
  TEST(bookmark->GetData().m_customName.empty(), ());

  // A normal save retains the recovered context for the next map-free startup.
  TEST(manager.SaveBookmarkCategory(category), ());
  auto const restored = LoadKmlFile(path, FileType::Kml);
  TEST(restored != nullptr, ());
  TEST(kml::HasBookmarkRegionData(restored->m_bookmarksData.front()), ());
}

UNIT_CLASS_TEST(Fixture, BookmarkNames_MapDownloadRecoversOnlyAffectedContext)
{
  auto & framework = GetFramework();
  auto & manager = framework.GetBookmarkManager();
  framework.SetMapLanguageCode("it");
  auto const category = manager.CreateBookmarkCategory("Downloaded", true /* autoSave */);
  auto legacy = MakeBookmark({{"default", "厦门大学"}, {"en", "Xiamen University"}});
  auto const point = legacy.m_point;
  legacy.m_properties.clear();
  auto elsewhere = legacy;
  elsewhere.m_point = mercator::FromLatLon(10, 10);
  auto const * recovered = manager.GetEditSession().CreateBookmark(std::move(legacy), category);
  auto const * other = manager.GetEditSession().CreateBookmark(std::move(elsewhere), category);
  auto const * captured =
      manager.GetEditSession().CreateBookmark(MakeBookmark({{"default", "鼓东路"}, {"en", "Gudong Road"}}), category);
  auto const path = manager.GetCategoryFileName(category);
  std::string before;
  FileReader(path).ReadAsString(before);

  place_page::BuildInfo selection;
  selection.m_userMarkId = other->GetId();
  selection.m_mercator = other->GetPivot();
  selection.m_match = place_page::BuildInfo::Match::UserMarkOnly;
  framework.BuildAndSetPlacePageInfo(selection);
  size_t updates = 0, notifications = 0;
  framework.SetPlacePageListeners({}, {}, [&]() { ++updates; }, {});
  manager.SetBookmarksChangedCallback([&]() { ++notifications; });

  auto const countryId = framework.GetCountryInfoGetter().GetRegionCountryId(point);
  TEST(!countryId.empty(), ());
  platform::LocalCountryFile country(GetPlatform().SettingsDir(), platform::CountryFile(countryId), 0);
  {
    generator::tests_support::TestMwmBuilder builder(country, feature::DataHeader::MapType::Country);
    builder.SetMwmLanguages({"zh"});
    generator::tests_support::TestMultilingualPOI poi(point, "厦门大学", {{"en", "Xiamen University"}});
    poi.SetTypes({{"amenity", "university"}});
    builder.Add(poi);
  }
  auto const localFile = std::make_shared<platform::LocalCountryFile>(country);
  framework.OnCountryFileDownloaded(country.GetCountryName(), localFile);
  TEST(kml::HasBookmarkRegionData(recovered->GetData()), ());
  TEST_EQUAL(recovered->GetPreferredName(), "Xiamen University", ());
  TEST(!kml::HasBookmarkRegionData(other->GetData()), ());
  TEST_EQUAL(captured->GetPreferredName(), "Gudong Road", ());
  TEST_EQUAL(notifications, 1, ());
  TEST_EQUAL(updates, 0, ("The selected bookmark is outside the downloaded map"));

  // A repeated map event has no new context, even while notifications are suspended.
  manager.SetNotificationsEnabled(false);
  framework.OnCountryFileDownloaded(country.GetCountryName(), localFile);
  manager.SetNotificationsEnabled(true);
  TEST_EQUAL(notifications, 1, ());
  TEST_EQUAL(updates, 0, ());
  std::string after;
  FileReader(path).ReadAsString(after);
  TEST_EQUAL(after, before, ());

  framework.SetMapLanguageCode("zh");
  TEST_EQUAL(recovered->GetPreferredName(), "厦门大学", ());
  TEST_EQUAL(captured->GetPreferredName(), "鼓东路", ());
  TEST_EQUAL(notifications, 2, ("Settings still refresh all names"));
  TEST_EQUAL(updates, 1, ());
  framework.SetPlacePageListeners({}, {}, {}, {});
}

UNIT_CLASS_TEST(Fixture, BookmarkNames_SearchRefreshesWithTransliteration)
{
  auto & framework = GetFramework();
  auto & manager = framework.GetBookmarkManager();
  framework.SetMapLanguageCode("en");
  // Keep the source language foreign to both the map and the device language used for indexing.
  bool const chineseDevice = languages::Normalize(languages::GetCurrentOrig()) == "zh";
  auto const original = chineseDevice ? "Тверская улица" : "鼓东路";
  auto const latin = chineseDevice ? "Tverskaya ulitsa" : "gu dong lù";
  auto const category = manager.CreateBookmarkCategory("Search", false /* autoSave */);
  auto const * bookmark = manager.GetEditSession().CreateBookmark(
      MakeBookmark({{"default", original}}, {chineseDevice ? "ru" : "zh"}), category);
  auto & api = framework.GetSearchAPI();
  api.OnViewportChanged(m2::RectD(-1, -1, 1, 1));
  manager.PrepareForSearch(category);
  auto search = [&](std::string const & query)
  {
    std::promise<std::vector<kml::MarkId>> promise;
    auto future = promise.get_future();
    search::BookmarksSearchParams params;
    params.m_query = query + " ";
    params.m_groupId = category;
    params.m_onResults = [&](auto results, auto status)
    {
      if (status == search::BookmarksSearchParams::Status::Completed)
        promise.set_value(std::move(results));
    };
    api.SearchInBookmarks(std::move(params));
    TEST(future.wait_for(std::chrono::seconds(5)) == std::future_status::ready, ());
    return future.get();
  };

  framework.AllowTransliteration(false);
  TEST(search(latin).empty(), ());
  framework.AllowTransliteration(true);
  TEST_EQUAL(bookmark->GetPreferredName(), latin, ());
  TEST_EQUAL(search(latin), std::vector<kml::MarkId>{bookmark->GetId()}, ());
  framework.AllowTransliteration(false);
  TEST(search(latin).empty(), ());
  TEST_EQUAL(search(original), std::vector<kml::MarkId>{bookmark->GetId()}, ());
}

UNIT_CLASS_TEST(Fixture, BookmarkNames_PlacePageAndSettings)
{
  auto & framework = GetFramework();
  auto & manager = framework.GetBookmarkManager();
  auto const category = manager.CreateBookmarkCategory("Names", false /* autoSave */);
  auto data = MakeBookmark({{"default", "鼓东路"}});
  auto const * bookmark = manager.GetEditSession().CreateBookmark(std::move(data), category);
  framework.SetMapLanguageCode("it");
  framework.AllowTransliteration(true);
  place_page::BuildInfo info;
  info.m_userMarkId = bookmark->GetId();
  info.m_mercator = bookmark->GetPivot();
  info.m_match = place_page::BuildInfo::Match::UserMarkOnly;
  framework.BuildAndSetPlacePageInfo(info);
  TEST_EQUAL(framework.GetCurrentPlacePageInfo().GetTitle(), "gu dong lù", ());
  TEST_EQUAL(framework.GetCurrentPlacePageInfo().GetSecondaryTitle(), "鼓东路", ());
  TEST_EQUAL(bookmark->GetPreferredName(), framework.GetCurrentPlacePageInfo().GetTitle(), ());

  size_t updates = 0;
  framework.SetPlacePageListeners({}, {}, [&]() { ++updates; }, {});
  framework.AllowTransliteration(false);
  TEST_EQUAL(framework.GetCurrentPlacePageInfo().GetTitle(), "鼓东路", ());
  TEST(framework.GetCurrentPlacePageInfo().GetSecondaryTitle().empty(), ());
  framework.AllowTransliteration(true);
  TEST_EQUAL(framework.GetCurrentPlacePageInfo().GetTitle(), "gu dong lù", ());
  framework.SetMapLanguageCode("zh");
  TEST_EQUAL(framework.GetCurrentPlacePageInfo().GetTitle(), "鼓东路", ());
  TEST_EQUAL(updates, 3, ());
  TEST(bookmark->GetData().m_customName.empty(), ("Settings are not a rename"));
  TEST_EQUAL(framework.GetCurrentPlacePageInfo().GetTitle(), bookmark->GetPreferredName(),
             ("The editor's displayed title agrees with its save-time comparison"));
  framework.SetPlacePageListeners({}, {}, {}, {});
}

UNIT_CLASS_TEST(Fixture, BookmarkNames_CaptureSourceAndRemoveMap)
{
  auto & framework = GetFramework();
  auto & manager = framework.GetBookmarkManager();
  auto const point = mercator::FromLatLon(24.437, 118.102);
  platform::LocalCountryFile country(GetPlatform().SettingsDir(), platform::CountryFile("BookmarkNamesChina"), 0);
  {
    generator::tests_support::TestMwmBuilder builder(country, feature::DataHeader::MapType::Country);
    builder.SetMwmLanguages({"zh"});
    generator::tests_support::TestMultilingualPOI poi(point, "厦门大学", {{"en", "Xiamen University"}});
    poi.SetTypes({{"amenity", "university"}});
    builder.Add(poi);
  }
  TEST_EQUAL(framework.RegisterMap(country).second, MwmSet::RegResult::Success, ());
  framework.SetMapLanguageCode("it");
  place_page::BuildInfo info;
  info.m_mercator = point;
  info.m_featureId = framework.GetFeatureAtPoint(point);
  TEST(info.m_featureId.IsValid(), ());
  framework.BuildAndSetPlacePageInfo(info);
  TEST_EQUAL(framework.GetCurrentPlacePageInfo().GetTitle(), "Xiamen University", ());

  // This is the same capture API used by Android and iOS.
  auto data = framework.GetCurrentPlacePageInfo().MakeBookmarkData();
  TEST_EQUAL(data.m_properties.at(kml::kBookmarkRegionLanguages), "zh", ());
  TEST_EQUAL(kml::GetDefaultStr(data.m_name), "厦门大学", ());
  TEST(!data.m_featureTypes.empty(), ());
  data.m_point = point;
  auto const category = manager.CreateBookmarkCategory("Captured", false /* autoSave */);
  auto const * bookmark = manager.GetEditSession().CreateBookmark(std::move(data), category);
  info = {};
  info.m_mercator = point;
  info.m_userMarkId = bookmark->GetId();
  framework.BuildAndSetPlacePageInfo(info);
  TEST_EQUAL(framework.GetCurrentPlacePageInfo().GetTitle(), "Xiamen University", ());
  TEST_EQUAL(framework.GetCurrentPlacePageInfo().GetSecondaryTitle(), "厦门大学", ());

  framework.DeregisterAllMaps();
  framework.AllowTransliteration(false);
  TEST_EQUAL(framework.GetCurrentPlacePageInfo().GetTitle(), "Xiamen University", ());
  TEST_EQUAL(bookmark->GetPreferredName(), "Xiamen University", ());
  framework.SetMapLanguageCode("zh");
  TEST_EQUAL(framework.GetCurrentPlacePageInfo().GetTitle(), "厦门大学", ());

  // A legacy feature bookmark in the ocean has no country context to recover.
  data = MakeBookmark({{"default", "Old name"}});
  data.m_properties.clear();
  data.m_point = mercator::FromLatLon(0, 0);
  auto const * unknown = manager.GetEditSession().CreateBookmark(std::move(data), category);
  TEST(!kml::HasBookmarkRegionData(unknown->GetData()), ());
  TEST_EQUAL(unknown->GetPreferredName(), "Old name", ());
}
}  // namespace bookmark_names_tests
