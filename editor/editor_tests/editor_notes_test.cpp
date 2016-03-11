#include "testing/testing.hpp"

#include "editor/editor_notes.hpp"

#include "geometry/mercator.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include "coding/file_name_utils.hpp"

#include "base/math.hpp"

using namespace editor;

UNIT_TEST(Notes_Smoke)
{
  auto const fileName = "notes.xml";
  auto const fullFileName = my::JoinFoldersToPath({GetPlatform().WritableDir()}, fileName);
  platform::tests_support::ScopedFile sf(fileName);
  {
    auto const notes = Notes::MakeNotes(fullFileName, true);
    notes->CreateNote({1, 2}, "Some note1");
    notes->CreateNote({2, 2}, "Some note2");
    notes->CreateNote({1, 1}, "Some note3");
  }
  {
    auto const notes = Notes::MakeNotes(fullFileName, true);
    auto const result = notes->GetNotes();
    TEST_EQUAL(result.size(), 3, ());
    vector<Note> const expected {
      {MercatorBounds::ToLatLon({1, 2}), "Some note1"},
      {MercatorBounds::ToLatLon({2, 2}), "Some note2"},
      {MercatorBounds::ToLatLon({1, 1}), "Some note3"}
    };
    for (auto i = 0; i < result.size(); ++i)
    {
      TEST(my::AlmostEqualAbs(result[i].m_point.lat, expected[i].m_point.lat, 1e-7), ());
      TEST(my::AlmostEqualAbs(result[i].m_point.lon, expected[i].m_point.lon, 1e-7), ());
      TEST_EQUAL(result[i].m_note, expected[i].m_note, ());
    }
  }
}
