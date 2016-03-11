#pragma once

#include "geometry/point2d.hpp"

#include "editor/server_api.hpp"

#include "std/mutex.hpp"
#include "std/shared_ptr.hpp"
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

class Notes : public enable_shared_from_this<Notes>
{
public:
  static shared_ptr<Notes> MakeNotes(string const & fileName);

  void CreateNote(m2::PointD const & point, string const & text);

  /// Uploads notes to the server in a separate thread.
  void Upload(osm::OsmOAuth const & auth);

  vector<Note> const & GetNotes() const { return m_notes; }

  uint32_t UnuploadedNotesCount() const { return m_notes.size(); }
  uint32_t UploadedNotesCount() const { return m_uploadedNotes; }

private:
  Notes(string const & fileName);

  bool Load();
  bool Save();

  string const m_fileName;
  mutable mutex m_mu;

  vector<Note> m_notes;

  uint32_t m_uploadedNotes;
};
}  // namespace
