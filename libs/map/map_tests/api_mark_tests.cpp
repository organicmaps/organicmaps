#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "map/api_mark_point.hpp"
#include "map/framework.hpp"
#include "map/share.hpp"

#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"

#include "platform/country_defines.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include <string>

namespace api_mark_tests
{
using df::test_support::VisualParamsFixture;
// Returns the single API mark parsed from an om:// deep link. ExecuteMapApiRequest()
// materializes the marks described by the URL into the API user-mark group.
ApiMarkPoint const * CreateSingleApiMark(Framework & fm, std::string const & url)
{
  TEST_EQUAL(fm.ParseAndSetApiURL(url), url_scheme::ParsedMapApi::UrlType::Map, (url));
  fm.ExecuteMapApiRequest();
  auto const & ids = fm.GetBookmarkManager().GetUserMarkIds(UserMark::Type::API);
  TEST_EQUAL(ids.size(), 1, (url));
  return fm.GetBookmarkManager().GetMark<ApiMarkPoint>(*ids.begin());
}

// The per-point "id" and the global "backurl" are independent API inputs: a request may
// carry either, both, or neither. FillApiMarkInfo() copies the mark's GetApiID() and
// GenerateApiBackUrl() into place_page::Info::GetApiId()/GetApiUrl() respectively -- the
// two values the place page surfaces, with the id returned to the caller (Android
// EXTRA_POINT_ID). This pins down that the id is surfaced independently of the back URL.
UNIT_TEST(ApiPoint_PointIdIsIndependentOfBackUrl)
{
  Framework fm(FrameworkParams(false /* m_enableDiffs */));

  // id without backurl: id present, back URL empty.
  {
    auto const * mark = CreateSingleApiMark(fm, "om://map?ll=1,2&id=foo");
    TEST_EQUAL(mark->GetApiID(), "foo", ());
    TEST(fm.GenerateApiBackUrl(*mark).empty(), ("No backurl -> empty back URL, id still present"));
  }

  // backurl without id: a back URL but no point id to echo back.
  {
    auto const * mark = CreateSingleApiMark(fm, "om://map?ll=1,2&backurl=testapp");
    TEST(mark->GetApiID().empty(), ("No id -> empty point id"));
    TEST(!fm.GenerateApiBackUrl(*mark).empty(), ());
  }

  // Both present: the point id and the back URL are distinct, separately surfaced fields.
  {
    auto const * mark = CreateSingleApiMark(fm, "om://map?ll=1,2&id=foo&backurl=testapp");
    TEST_EQUAL(mark->GetApiID(), "foo", ());
    TEST_NOT_EQUAL(mark->GetApiID(), fm.GenerateApiBackUrl(*mark), ("point id and back URL are distinct"));
  }
}

UNIT_CLASS_TEST(VisualParamsFixture, ApiPoint_FeatureMatchingIsOptOut)
{
  using namespace generator::tests_support;
  Framework fm({} /* params */, false /* loadMaps */);
  platform::LocalCountryFile country(GetPlatform().WritableDir(), platform::CountryFile("ApiPointPark"), 0);
  platform::CountryIndexes::DeleteFromDisk(country);
  country.DeleteFromDisk(MapFileType::Map);
  {
    TestPark park({{0, 0}, {2, 0}, {2, 2}, {0, 2}, {0, 0}}, "Enclosing park", "en");
    TestPOI poi(mercator::FromLatLon(0.5, 0.5), "Existing cafe", "en");
    auto const center = mercator::FromLatLon(1.5, 1.5);
    TestBuilding building(m2::RectD(center.x - 0.001, center.y - 0.001, center.x + 0.001, center.y + 0.001),
                          "Existing building", "1", "", "en");
    TestMwmBuilder builder(country, feature::DataHeader::MapType::Country);
    for (TestFeature * feature : std::initializer_list<TestFeature *>{&park, &poi, &building})
    {
      feature->GetMetadata().Set(feature::Metadata::FMD_WIKIPEDIA, "en:Test_feature");
      feature->GetMetadata().Set(feature::Metadata::FMD_OPEN_HOURS, "24/7");
      builder.Add(*feature);
    }
  }
  TEST_EQUAL(fm.RegisterMap(country).second, MwmSet::RegResult::Success, ());

  auto checkMark = [&](ApiMarkPoint const & mark, bool matchFeature, std::string const & name)
  {
    auto const point = mark.GetPivot();
    auto const featureId = fm.GetFeatureAtPoint(point);
    TEST(featureId.IsValid(), ("A control feature must exist at the shared coordinate."));
    TEST_EQUAL(mark.ShouldMatchFeature(), matchFeature, ());
    place_page::BuildInfo bi;
    bi.m_source = place_page::BuildInfo::Source::User;
    bi.m_mercator = point;
    bi.m_userMarkId = mark.GetId();
    fm.BuildAndSetPlacePageInfo(bi);
    auto const & info = fm.GetCurrentPlacePageInfo();
    TEST_EQUAL(info.GetID().IsValid(), matchFeature, ());
    if (matchFeature)
      TEST_EQUAL(info.GetID(), featureId, ());
    TEST_EQUAL(info.GetTypes().Empty(), !matchFeature, ());
    TEST_EQUAL(info.GetMetadata(feature::Metadata::FMD_WIKIPEDIA).empty(), !matchFeature, ());
    TEST_EQUAL(info.GetMetadata(feature::Metadata::FMD_OPEN_HOURS).empty(), !matchFeature, ());
    TEST_ALMOST_EQUAL_ABS(info.GetMercator(), point, 1e-7, ());
    TEST_EQUAL(info.GetApiId(), mark.GetApiID(), ());
    TEST_EQUAL(info.GetApiUrl(), fm.GenerateApiBackUrl(mark), ());
    if (!name.empty())
      TEST_EQUAL(info.GetTitle(), name, ());
  };

  // Area, coincident POI and building: matching depends on the caller's policy, not geometry.
  for (auto const * ll : {"1,1", "0.5,0.5", "1.5,1.5"})
  {
    for (auto const * name : {"", "&n=Shared%20point"})
    {
      for (bool matchFeature : {true, false})
      {
        auto const link = std::string("om://map?ll=") + ll + "&id=external&backurl=testapp" + name +
                          (matchFeature ? "" : "&match=none");
        auto const * mark = CreateSingleApiMark(fm, link);
        checkMark(*mark, matchFeature, *name ? "Shared point" : "");
      }
    }
  }

  // Exercise the actual native share builder, including a renamed meeting point inside the park.
  for (auto const & ll : {ms::LatLon(1, 1), ms::LatLon(0.5, 0.5), ms::LatLon(1.5, 1.5)})
  {
    share::Place place;
    place.m_ll = ll;
    place.m_name = "We should meet here";
    place.m_zoom = 17;
    auto const * mark = CreateSingleApiMark(fm, share::Build(place, {}).m_url);
    checkMark(*mark, true, place.m_name);
  }

  // Each pin keeps its policy when selected again after another pin in the same request.
  TEST_EQUAL(fm.ParseAndSetApiURL("om://map?ll=1,1&n=Imported&id=plain&match=none&ll=1,1&n=Meeting&id=matched"),
             url_scheme::ParsedMapApi::UrlType::Map, ());
  fm.ExecuteMapApiRequest();
  auto const & ids = fm.GetBookmarkManager().GetUserMarkIds(UserMark::Type::API);
  TEST_EQUAL(ids.size(), 2, ());
  for (int repeat = 0; repeat < 2; ++repeat)
  {
    for (auto const id : ids)
    {
      auto const * mark = fm.GetBookmarkManager().GetMark<ApiMarkPoint>(id);
      checkMark(*mark, mark->GetApiID() == "matched", mark->GetName());
    }
  }

  fm.DeregisterAllMaps();
  platform::CountryIndexes::DeleteFromDisk(country);
  country.DeleteFromDisk(MapFileType::Map);
}
}  // namespace api_mark_tests
