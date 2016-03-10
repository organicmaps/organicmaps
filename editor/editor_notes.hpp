#pragma once

#include "geometry/point2d.hpp"

#include "std/mutex.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace editor
{
struct Note
{
  Note(m2::PointD const & point, string const & text)
      : m_point(point),
        m_note(text)
  {
  }

  m2::PointD m_point;
  string m_note;
};

inline bool operator==(Note const & a, Note const & b)
{
  return a.m_point == b.m_point && b.m_note == b.m_note;
}

class Notes
{
public:
  Notes(string const & fileName);

  void CreateNote(m2::PointD const & point, string const & text);
  void Upload();

  vector<Note> GetNotes() const { return m_notes; }

  uint32_t UnuploadedNotesCount() const { return m_notes.size(); }
  uint32_t UploadedNotesCount() const { return m_uploadedNotes; }

private:
  bool Load();
  bool Save();

  string const m_fileName;
  mutable mutex m_mu;

  vector<Note> m_notes;

  uint32_t m_uploadedNotes;
};
}  // namespace
