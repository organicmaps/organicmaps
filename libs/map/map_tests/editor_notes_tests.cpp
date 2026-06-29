#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "map/framework.hpp"
#include "map/place_page_info.hpp"

#include "drape_frontend/visual_params.hpp"

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

#include <string>
#include <utility>

namespace editor_notes_tests
{
using namespace generator::tests_support;
using platform::LocalCountryFile;
using platform::tests_support::ScopedFile;

// https://github.com/organicmaps/organicmaps/issues/11170
// A note created via "Edit place" on a line/area feature must be attached to the user's actual map
// selection (tap) point, not to the feature's geometric center (which for a road or forest can be
// far from where the problem was reported).
UNIT_TEST(EditorNotes_UseSelectionPointNotFeatureCenter)
{
  // The selection point is copied verbatim, so it must match the tap exactly. Coordinates read back
  // from the mwm are quantized and notes round-trip through XML, hence the looser tolerance for those.
  double constexpr kExactEps = 1e-7;
  double constexpr kGeomEps = 1e-4;

  // A straight horizontal road; its polyline center is the midpoint (0.5, 0).
  m2::PointD const roadCenter(0.5, 0.0);

  // Construct the framework first: it loads the classificator required to build test features.
  Framework frm({} /* params */, false /* loadMaps */);
  df::VisualParams::Init(1.0, 1024);

  std::string const kCountryName = "EditorNotesLand";
  LocalCountryFile localFile(GetPlatform().WritableDir(), platform::CountryFile(kCountryName), 0 /* version */);
  platform::CountryIndexes::DeleteFromDisk(localFile);
  localFile.DeleteFromDisk(MapFileType::Map);

  {
    TestMwmBuilder builder(localFile, feature::DataHeader::MapType::Country);
    builder.Add(TestStreet({{0.0, 0.0}, {1.0, 0.0}}, "Test Road", "en"));
  }

  auto const regResult = frm.RegisterMap(localFile);
  TEST_EQUAL(regResult.second, MwmSet::RegResult::Success, ());

  // Tap on the road far from its center, as if reporting a problem at that exact spot.
  m2::PointD const tapPoint(0.9, 0.0);
  {
    place_page::BuildInfo bi;
    bi.m_source = place_page::BuildInfo::Source::User;
    bi.m_mercator = tapPoint;
    frm.BuildAndSetPlacePageInfo(bi);
  }

  TEST(frm.HasPlacePageInfo(), ());
  auto const & info = frm.GetCurrentPlacePageInfo();
  TEST(info.GetID().IsValid(), ("The road should be selected by the tap."));
  // The selection keeps the tap point, not the road center.
  TEST(info.GetMercator().EqualDxDy(tapPoint, kExactEps), (info.GetMercator(), tapPoint));

  // The editable map object (what the note used before the fix) carries the road center,
  // which is far away from the tap point.
  osm::EditableMapObject emo;
  TEST(frm.GetEditableMapObject(info.GetID(), emo), ());
  TEST(emo.GetMercator().EqualDxDy(roadCenter, kGeomEps), (emo.GetMercator(), roadCenter));
  TEST(!emo.GetMercator().EqualDxDy(tapPoint, kGeomEps), (emo.GetMercator()));

  // Create a note and verify it is attached to the tap point, not the road center.
  ScopedFile const notesFile("test_editor_notes.xml", ScopedFile::Mode::DoNotCreate);
  osm::Editor::Instance().SetNotesForTesting(notesFile.GetFullPath());

  frm.CreateNote(emo, osm::Editor::NoteProblemType::General, "Obstacle on the road");

  editor::Notes const notes(notesFile.GetFullPath());
  auto const & stored = notes.GetNotesForTests();
  TEST_EQUAL(stored.size(), 1, ());
  TEST(stored.front().m_point.EqualDxDy(mercator::ToLatLon(tapPoint), kGeomEps),
       (stored.front().m_point, mercator::ToLatLon(tapPoint)));
  TEST(!stored.front().m_point.EqualDxDy(mercator::ToLatLon(roadCenter), kGeomEps), (stored.front().m_point));

  platform::CountryIndexes::DeleteFromDisk(localFile);
  localFile.DeleteFromDisk(MapFileType::Map);
}
}  // namespace editor_notes_tests
