#pragma once

#include "editor/server_api.hpp"

#include "geometry/latlon.hpp"

#include "base/macros.hpp"

#include <list>
#include <memory>
#include <mutex>
#include <string>

namespace editor
{
struct Note
{
  Note(ms::LatLon const & point, std::string const & text) : m_point(point), m_note(text) {}
  ms::LatLon m_point;
  std::string m_note;
};

inline bool operator==(Note const & lhs, Note const & rhs)
{
  return lhs.m_point == rhs.m_point && lhs.m_note == rhs.m_note;
}

class Notes : public std::enable_shared_from_this<Notes>
{
public:
  static float constexpr kTolerance = 1e-7;
  static std::shared_ptr<Notes> MakeNotes(std::string const & fileName = "notes.xml", bool const fullPath = false);

  void CreateNote(ms::LatLon const & latLon, std::string const & text);

  /// Uploads notes to the server in a separate thread.
  /// Called on main thread from system event.
  void Upload(osm::OsmOAuth const & auth);

  std::list<Note> GetNotes() const;

  size_t NotUploadedNotesCount() const;
  size_t UploadedNotesCount() const;

private:
  explicit Notes(std::string const & fileName);

  std::string const m_fileName;
  mutable std::mutex m_mu;

  // m_notes keeps the notes that have not been uploaded yet.
  // Once a note has been uploaded, it is removed from m_notes.
  std::list<Note> m_notes;

  uint32_t m_uploadedNotesCount = 0;

  DISALLOW_COPY_AND_MOVE(Notes);
};
}  // namespace editor
