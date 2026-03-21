#include "testing/testing.hpp"

#include "editor/editor_notes.hpp"

#include "geometry/mercator.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

using platform::tests_support::ScopedFile;

UNIT_TEST(Notes_Smoke)
{
  ScopedFile sf("test_notes.xml", ScopedFile::Mode::DoNotCreate);

  std::vector<editor::Note> const expected{{mercator::ToLatLon({1, 2}), "Some note1"},
                                           {mercator::ToLatLon({2, 2}), "Some note2"},
                                           {mercator::ToLatLon({1, 1}), "Some note3"}};

  {
    editor::Notes notes(sf.GetFullPath());
    for (auto const & e : expected)
      notes.CreateNote(e.m_point, e.m_note);
  }

  {
    editor::Notes notes(sf.GetFullPath());
    auto const result = notes.GetNotesForTests();
    TEST_EQUAL(result.size(), 3, ());

    TEST(std::equal(result.begin(), result.end(), expected.begin(),
                    [](editor::Note const & lhs, editor::Note const & rhs)
    { return lhs.m_point.EqualDxDy(rhs.m_point, editor::Notes::kTolerance); }),
         ());
  }
}
