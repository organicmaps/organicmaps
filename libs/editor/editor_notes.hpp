#pragma once

#include "editor/osm_auth.hpp"

#include "geometry/latlon.hpp"

#include "base/macros.hpp"

#include <list>
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

class Notes
{
public:
  explicit Notes(std::string const & filePath);

  static float constexpr kTolerance = 1.0E-7;

  void CreateNote(ms::LatLon const & latLon, std::string const & text);

  /// Uploads notes to the server in a separate thread.
  /// Called on main thread from system event.
  void Upload(osm::OsmOAuth const & auth);

  std::list<Note> const & GetNotesForTests() const { return m_notes; }

  size_t NotUploadedNotesCount() const;
  size_t UploadedNotesCount() const;

private:
  std::string const m_filePath;
  mutable std::mutex m_mu;

  // m_notes keeps the notes that have not been uploaded yet.
  // Once a note has been uploaded, it is removed from m_notes.
  std::list<Note> m_notes;

  uint32_t m_uploadedNotesCount = 0;

  std::atomic<bool> m_isUploadingNow = false;

  DISALLOW_COPY_AND_MOVE(Notes);
};
}  // namespace editor
