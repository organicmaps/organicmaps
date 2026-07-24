#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "map/framework.hpp"
#include "map/place_page_info.hpp"

#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"

#include "editor/editor_notes.hpp"
#include "editor/osm_editor.hpp"

#include "indexer/data_header.hpp"
#include "indexer/editable_map_object.hpp"
#include "indexer/mwm_set.hpp"

#include "platform/country_defines.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

namespace editor_notes_tests
{
using namespace generator::tests_support;
using df::test_support::VisualParamsFixture;
using platform::LocalCountryFile;
using platform::tests_support::ScopedFile;

// https://github.com/organicmaps/organicmaps/issues/11170
// A note created via "Edit place" on a line/area feature must be attached to the user's map
// selection (tap) point, not to the feature's geometric center, which for a road or forest can be
// far from where the problem was reported.
UNIT_CLASS_TEST(VisualParamsFixture, EditorNotes_UseSelectionPointNotFeatureCenter)
{
  Framework frm({} /* params */, false /* loadMaps */);

  // A straight horizontal road; its geometric center is the midpoint (0.5, 0).
  LocalCountryFile country(GetPlatform().WritableDir(), platform::CountryFile("EditorNotesLand"), 0 /* version */);
  platform::CountryIndexes::DeleteFromDisk(country);
  country.DeleteFromDisk(MapFileType::Map);
  {
    TestMwmBuilder builder(country, feature::DataHeader::MapType::Country);
    builder.Add(TestStreet({{0.0, 0.0}, {1.0, 0.0}}, "Test Road", "en"));
  }
  TEST_EQUAL(frm.RegisterMap(country).second, MwmSet::RegResult::Success, ());

  // Tap on the road far from its center, as if reporting a problem at that exact spot.
  m2::PointD const tapPoint(0.9, 0.0);
  place_page::BuildInfo bi;
  bi.m_source = place_page::BuildInfo::Source::User;
  bi.m_mercator = tapPoint;
  frm.BuildAndSetPlacePageInfo(bi);

  auto const & info = frm.GetCurrentPlacePageInfo();
  TEST(info.GetID().IsValid(), ("The tap must select the road."));
  // The selection keeps the tap point verbatim, not the road center.
  TEST_ALMOST_EQUAL_ABS(info.GetMercator(), tapPoint, 1e-7, ());

  ScopedFile const notesFile("test_editor_notes.xml", ScopedFile::Mode::DoNotCreate);
  osm::Editor::Instance().SetNotesForTesting(notesFile.GetFullPath());

  osm::EditableMapObject emo;
  TEST(frm.GetEditableMapObject(info.GetID(), emo), ());
  frm.CreateNote(emo, osm::Editor::NoteProblemType::General, "Obstacle on the road");

  editor::Notes const notes(notesFile.GetFullPath());
  TEST_EQUAL(notes.GetNotesForTests().size(), 1, ());
  // Looser tolerance: the note round-trips through XML.
  TEST_ALMOST_EQUAL_ABS(notes.GetNotesForTests().front().m_point, mercator::ToLatLon(tapPoint), 1e-4, ());

  platform::CountryIndexes::DeleteFromDisk(country);
  country.DeleteFromDisk(MapFileType::Map);
}

UNIT_CLASS_TEST(VisualParamsFixture, EditorNotes_UseSelectionPointForExplicitFeatureTap)
{
  Framework frm({} /* params */, false /* loadMaps */);

  LocalCountryFile country(GetPlatform().WritableDir(), platform::CountryFile("EditorNotesExplicitLand"),
                           0 /* version */);
  platform::CountryIndexes::DeleteFromDisk(country);
  country.DeleteFromDisk(MapFileType::Map);
  {
    TestMwmBuilder builder(country, feature::DataHeader::MapType::Country);
    builder.Add(TestStreet({{0.0, 0.0}, {1.0, 0.0}}, "Test Road", "en"));
  }
  TEST_EQUAL(frm.RegisterMap(country).second, MwmSet::RegResult::Success, ());

  m2::PointD const tapPoint(0.9, 0.0);
  auto const roadId = frm.GetFeatureAtPoint(tapPoint);
  TEST(roadId.IsValid(), ("The tap must resolve the test road."));

  place_page::BuildInfo bi;
  bi.m_source = place_page::BuildInfo::Source::User;
  bi.m_mercator = tapPoint;
  bi.m_featureId = roadId;
  frm.BuildAndSetPlacePageInfo(bi);

  auto const & info = frm.GetCurrentPlacePageInfo();
  TEST_EQUAL(info.GetID(), roadId, ());
  TEST_EQUAL(info.GetGeomType(), feature::GeomType::Line, ());
  TEST_NOT_EQUAL(info.GetMercator(), tapPoint,
                 ("Explicit feature selection keeps the feature center in the place page."));

  ScopedFile const notesFile("test_editor_notes_explicit.xml", ScopedFile::Mode::DoNotCreate);
  osm::Editor::Instance().SetNotesForTesting(notesFile.GetFullPath());

  osm::EditableMapObject emo;
  TEST(frm.GetEditableMapObject(info.GetID(), emo), ());
  frm.CreateNote(emo, osm::Editor::NoteProblemType::General, "Obstacle on the road");

  editor::Notes const notes(notesFile.GetFullPath());
  TEST_EQUAL(notes.GetNotesForTests().size(), 1, ());
  TEST_ALMOST_EQUAL_ABS(notes.GetNotesForTests().front().m_point, mercator::ToLatLon(tapPoint), 1e-4, ());

  platform::CountryIndexes::DeleteFromDisk(country);
  country.DeleteFromDisk(MapFileType::Map);
}
}  // namespace editor_notes_tests
