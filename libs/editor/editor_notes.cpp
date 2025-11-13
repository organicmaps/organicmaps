#include "editor/editor_notes.hpp"
#include "editor/server_api.hpp"

#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include <pugixml.hpp>

namespace editor
{
namespace
{
bool LoadFromXml(pugi::xml_document const & xml, std::list<editor::Note> & notes, uint32_t & uploadedNotesCount)
{
  uint64_t notesCount;
  auto const root = xml.child("notes");
  if (!strings::to_uint64(root.attribute("uploadedNotesCount").value(), notesCount))
  {
    LOG(LERROR, ("Can't read uploadedNotesCount from file."));
    uploadedNotesCount = 0;
  }
  else
  {
    uploadedNotesCount = static_cast<uint32_t>(notesCount);
  }

  for (auto const & xNode : root.select_nodes("note"))
  {
    ms::LatLon latLon;

    auto const node = xNode.node();
    auto const lat = node.attribute("lat");
    if (!lat || !strings::to_double(lat.value(), latLon.m_lat))
      continue;

    auto const lon = node.attribute("lon");
    if (!lon || !strings::to_double(lon.value(), latLon.m_lon))
      continue;

    auto const text = node.attribute("text");
    if (!text)
      continue;

    notes.emplace_back(latLon, text.value());
  }
  return true;
}

void SaveToXml(std::list<editor::Note> const & notes, pugi::xml_document & xml, uint32_t const uploadedNotesCount)
{
  auto constexpr kDigitsAfterComma = 7;
  auto root = xml.append_child("notes");
  root.append_attribute("uploadedNotesCount") = uploadedNotesCount;
  for (auto const & note : notes)
  {
    auto node = root.append_child("note");

    node.append_attribute("lat") = strings::to_string_dac(note.m_point.m_lat, kDigitsAfterComma).data();
    node.append_attribute("lon") = strings::to_string_dac(note.m_point.m_lon, kDigitsAfterComma).data();
    node.append_attribute("text") = note.m_note.data();
  }
}

/// Not thread-safe, use only for initialization.
bool Load(std::string const & fileName, std::list<editor::Note> & notes, uint32_t & uploadedNotesCount)
{
  std::string content;
  try
  {
    auto const reader = GetPlatform().GetReader(fileName);
    reader->ReadAsString(content);
  }
  catch (FileAbsentException const &)
  {
    // It's normal if no notes file is present.
    return true;
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Can't process file.", fileName, e.Msg()));
    return false;
  }

  pugi::xml_document xml;
  if (!xml.load_buffer(content.data(), content.size()))
  {
    LOG(LERROR, ("Can't load notes, XML is ill-formed.", content));
    return false;
  }

  notes.clear();
  if (!LoadFromXml(xml, notes, uploadedNotesCount))
  {
    LOG(LERROR, ("Can't load notes, file is ill-formed.", content));
    return false;
  }

  return true;
}

/// Not thread-safe, use synchronization.
bool Save(std::string const & fileName, std::list<editor::Note> const & notes, uint32_t const uploadedNotesCount)
{
  pugi::xml_document xml;
  SaveToXml(notes, xml, uploadedNotesCount);
  return base::WriteToTempAndRenameToFile(
      fileName, [&xml](std::string const & fileName) { return xml.save_file(fileName.data(), "  "); });
}
}  // namespace

Notes::Notes(std::string const & filePath) : m_filePath(filePath)
{
  Load(m_filePath, m_notes, m_uploadedNotesCount);
}

void Notes::CreateNote(ms::LatLon const & latLon, std::string const & text)
{
  if (text.empty())
  {
    LOG(LWARNING, ("Attempt to create empty note"));
    return;
  }

  if (!mercator::ValidLat(latLon.m_lat) || !mercator::ValidLon(latLon.m_lon))
  {
    LOG(LWARNING, ("A note attached to a wrong latLon", latLon));
    return;
  }

  std::lock_guard<std::mutex> g(m_mu);
  auto const it = std::find_if(m_notes.begin(), m_notes.end(), [&latLon, &text](Note const & note)
  { return latLon.EqualDxDy(note.m_point, kTolerance) && text == note.m_note; });
  // No need to add the same note. It works in case when saved note are not uploaded yet.
  if (it != m_notes.end())
    return;

  m_notes.emplace_back(latLon, text);
  Save(m_filePath, m_notes, m_uploadedNotesCount);
}

void Notes::Upload(osm::OsmOAuth const & auth)
{
  size_t const toUpload = NotUploadedNotesCount();
  LOG(LINFO, ("Notes =", toUpload));
  if (toUpload == 0)
    return;

  if (m_isUploadingNow)
    return;
  m_isUploadingNow = true;

  GetPlatform().RunTask(Platform::Thread::Network, [this, auth]()
  {
    SCOPE_GUARD(resetUploadingFlag, [this]() { m_isUploadingNow = false; });

    /// @todo By VNG: unlock-lock is made below. Can rewrite better.
    std::unique_lock<std::mutex> ulock(m_mu);

    // Size of m_notes is decreased only in this method.
    auto & notes = m_notes;
    size_t size = notes.size();
    osm::ServerApi06 api(auth);

    while (size > 0)
    {
      try
      {
        ulock.unlock();
        /// @todo It looks like a race, but we do push_back (and getters) only in other _modifying_ places.
        /// size can only increase, front is not changed and list doesn't have reallocs.
        /// As I said before, refactor in a more _natural_ way with producer-consumer on a circular buffer pattern.
        auto const id = api.CreateNote(notes.front().m_point, notes.front().m_note);
        ulock.lock();

        LOG(LINFO, ("A note uploaded with id", id));
      }
      catch (osm::ServerApi06::ServerApi06Exception const & e)
      {
        LOG(LERROR, ("Can't upload note.", e.Msg()));
        // We believe that next iterations will suffer from the same error.
        return;
      }

      notes.pop_front();
      --size;
      ++m_uploadedNotesCount;
      Save(m_filePath, notes, m_uploadedNotesCount);
    }
  });
}

size_t Notes::NotUploadedNotesCount() const
{
  std::lock_guard<std::mutex> g(m_mu);
  return m_notes.size();
}

size_t Notes::UploadedNotesCount() const
{
  std::lock_guard<std::mutex> g(m_mu);
  return m_uploadedNotesCount;
}
}  // namespace editor
