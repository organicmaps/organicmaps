#include "testing/testing.hpp"

#include "editor/editor_notes.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

using namespace editor;

namespace editor
{
string DebugPrint(Note const & note)
{
  return "Note(" + DebugPrint(note.m_point) + ", \"" + note.m_note + "\")";
}
}  // namespace editor

UNIT_TEST(Notes)
{
  auto const fileName = "notes.xml";
  auto const fullFileName = Platform::PathJoin({GetPlatform().WritableDir(),
                                                fileName});
  platform::tests_support::ScopedFile sf(fileName);
  {
    auto const notes = Notes::MakeNotes(fullFileName);
    notes->CreateNote({1, 2}, "Some note1");
    notes->CreateNote({2, 2}, "Some note2");
    notes->CreateNote({1, 1}, "Some note3");
  }
  {
    auto const notes = Notes::MakeNotes(fullFileName);
    auto const result = notes->GetNotes();
    TEST_EQUAL(result.size(), 3, ());
    TEST_EQUAL(result,
               (vector<Note>{
                 {{1, 2}, "Some note1"},
                 {{2, 2}, "Some note2"},
                 {{1, 1}, "Some note3"}
               }), ());
  }
}
