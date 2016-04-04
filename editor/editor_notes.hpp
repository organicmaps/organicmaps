#pragma once

#include "editor/server_api.hpp"

#include "geometry/latlon.hpp"

#include "base/macros.hpp"

#include "std/list.hpp"
#include "std/mutex.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"

namespace editor
{
struct Note
{
  Note(ms::LatLon const & point, string const & text) : m_point(point), m_note(text) {}
  ms::LatLon m_point;
  string m_note;
};

inline bool operator==(Note const & a, Note const & b)
{
  return a.m_point == b.m_point && b.m_note == b.m_note;
}

class Notes : public enable_shared_from_this<Notes>
{
public:
  static shared_ptr<Notes> MakeNotes(string const & fileName = "notes.xml",
                                     bool const fullPath = false);

  void CreateNote(ms::LatLon const & latLon, string const & text);

  /// Uploads notes to the server in a separate thread.
  void Upload(osm::OsmOAuth const & auth);

  vector<Note> const GetNotes() const;

  uint32_t NotUploadedNotesCount() const;
  uint32_t UploadedNotesCount() const;

private:
  Notes(string const & fileName);

  string const m_fileName;
  mutable mutex m_mu;

  // m_notes keeps the notes that have not been uploaded yet.
  // Once a note has been uploaded, it is removed from m_notes.
  list<Note> m_notes;

  uint32_t m_uploadedNotesCount = 0;

  DISALLOW_COPY_AND_MOVE(Notes);
};
}  // namespace
