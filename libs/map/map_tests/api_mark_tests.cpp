#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "map/api_mark_point.hpp"
#include "map/framework.hpp"

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

UNIT_CLASS_TEST(VisualParamsFixture, ApiPoint_DoesNotInheritEnclosingFeature)
{
  using namespace generator::tests_support;
  Framework fm({} /* params */, false /* loadMaps */);
  platform::LocalCountryFile country(GetPlatform().WritableDir(), platform::CountryFile("ApiPointPark"), 0);
  platform::CountryIndexes::DeleteFromDisk(country);
  country.DeleteFromDisk(MapFileType::Map);
  {
    TestPark park({{0, 0}, {2, 0}, {2, 2}, {0, 2}, {0, 0}}, "Enclosing park", "en");
    park.GetMetadata().Set(feature::Metadata::FMD_WIKIPEDIA, "en:National_park");
    TestMwmBuilder builder(country, feature::DataHeader::MapType::Country);
    builder.Add(park);
  }
  TEST_EQUAL(fm.RegisterMap(country).second, MwmSet::RegResult::Success, ());
  auto const point = mercator::FromLatLon(1, 1);
  TEST(fm.GetFeatureAtPoint(point).IsValid(), ("The enclosing feature must be present."));

  for (auto const * name : {"", "&n=Shared%20point"})
  {
    auto const * mark = CreateSingleApiMark(fm, std::string("om://map?ll=1,1&id=external&backurl=testapp") + name);
    place_page::BuildInfo bi;
    bi.m_source = place_page::BuildInfo::Source::User;
    bi.m_mercator = point;
    bi.m_userMarkId = mark->GetId();
    fm.BuildAndSetPlacePageInfo(bi);
    auto const & info = fm.GetCurrentPlacePageInfo();
    TEST(!info.GetID().IsValid(), ("An external API point does not identify an OSM feature."));
    TEST(info.GetMetadata(feature::Metadata::FMD_WIKIPEDIA).empty(), ());
    TEST_ALMOST_EQUAL_ABS(info.GetMercator(), point, 1e-7, ());
    TEST_EQUAL(info.GetApiId(), "external", ());
    TEST_EQUAL(info.GetApiUrl(), fm.GenerateApiBackUrl(*mark), ());
    if (*name)
      TEST_EQUAL(info.GetTitle(), "Shared point", ());
  }

  fm.DeregisterAllMaps();
  platform::CountryIndexes::DeleteFromDisk(country);
  country.DeleteFromDisk(MapFileType::Map);
}
}  // namespace api_mark_tests
