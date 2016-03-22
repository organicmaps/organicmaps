#include "editor/editor_notes.hpp"

#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "std/future.hpp"

#include "3party/pugixml/src/pugixml.hpp"

namespace
{
bool LoadFromXml(pugi::xml_document const & xml, list<editor::Note> & notes,
                 uint32_t & uploadedNotesCount)
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

  for (auto const xNode : root.select_nodes("note"))
  {
    ms::LatLon latLon;

    auto const node = xNode.node();
    auto const lat = node.attribute("lat");
    if (!lat || !strings::to_double(lat.value(), latLon.lat))
      continue;

    auto const lon = node.attribute("lon");
    if (!lon || !strings::to_double(lon.value(), latLon.lon))
      continue;

    auto const text = node.attribute("text");
    if (!text)
      continue;

    notes.emplace_back(latLon, text.value());
  }
  return true;
}

void SaveToXml(list<editor::Note> const & notes, pugi::xml_document & xml,
               uint32_t const uploadedNotesCount)
{
  auto constexpr kDigitsAfterComma = 7;
  auto root = xml.append_child("notes");
  root.append_attribute("uploadedNotesCount") = uploadedNotesCount;
  for (auto const & note : notes)
  {
    auto node = root.append_child("note");

    node.append_attribute("lat") =
        strings::to_string_dac(note.m_point.lat, kDigitsAfterComma).data();
    node.append_attribute("lon") =
        strings::to_string_dac(note.m_point.lon, kDigitsAfterComma).data();
    node.append_attribute("text") = note.m_note.data();
  }
}

/// Not thread-safe, use only for initialization.
bool Load(string const & fileName, list<editor::Note> & notes, uint32_t & uploadedNotesCount)
{
  string content;
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
bool Save(string const & fileName, list<editor::Note> const & notes,
          uint32_t const uploadedNotesCount)
{
  pugi::xml_document xml;
  SaveToXml(notes, xml, uploadedNotesCount);
  return my::WriteToTempAndRenameToFile(
      fileName, [&xml](string const & fileName) { return xml.save_file(fileName.data(), "  "); });
}
}  // namespace

namespace editor
{
shared_ptr<Notes> Notes::MakeNotes(string const & fileName, bool const fullPath)
{
  return shared_ptr<Notes>(
      new Notes(fullPath ? fileName : GetPlatform().WritablePathForFile(fileName)));
}

Notes::Notes(string const & fileName) : m_fileName(fileName)
{
  Load(m_fileName, m_notes, m_uploadedNotesCount);
}

void Notes::CreateNote(m2::PointD const & point, string const & text)
{
  if (text.empty())
  {
    LOG(LWARNING, ("Attempt to create empty note"));
    return;
  }

  if (!MercatorBounds::ValidX(point.x) || !MercatorBounds::ValidY(point.y))
  {
    LOG(LWARNING, ("A note attached to a wrong point", point));
    return;
  }

  lock_guard<mutex> g(m_mu);
  m_notes.emplace_back(MercatorBounds::ToLatLon(point), text);
  Save(m_fileName, m_notes, m_uploadedNotesCount);
}

void Notes::Upload(osm::OsmOAuth const & auth)
{
  // Capture self to keep it from destruction until this thread is done.
  auto const self = shared_from_this();

  auto const doUpload = [self, auth]() {
    size_t size;

    {
      lock_guard<mutex> g(self->m_mu);
      size = self->m_notes.size();
    }

    osm::ServerApi06 api(auth);
    auto it = begin(self->m_notes);
    for (size_t i = 0; i != size; ++i, ++it)
    {
      try
      {
        auto const id = api.CreateNote(it->m_point, it->m_note);
        LOG(LINFO, ("A note uploaded with id", id));
      }
      catch (osm::ServerApi06::ServerApi06Exception const & e)
      {
        LOG(LERROR, ("Can't upload note.", e.Msg()));
        // We believe that next iterations will suffer from the same error.
        return;
      }

      lock_guard<mutex> g(self->m_mu);
      it = self->m_notes.erase(it);
      ++self->m_uploadedNotesCount;
      Save(self->m_fileName, self->m_notes, self->m_uploadedNotesCount);
    }
  };

  // Do not run more than one upload thread at a time.
  static auto future = async(launch::async, doUpload);
  auto const status = future.wait_for(milliseconds(0));
  if (status == future_status::ready)
    future = async(launch::async, doUpload);
}

vector<Note> const Notes::GetNotes() const
{
  lock_guard<mutex> g(m_mu);
  return {begin(m_notes), end(m_notes)};
}

uint32_t Notes::NotUploadedNotesCount() const
{
  lock_guard<mutex> g(m_mu);
  return m_notes.size();
}

uint32_t Notes::UploadedNotesCount() const
{
  lock_guard<mutex> g(m_mu);
  return m_uploadedNotesCount;
}
}  // namespace editor
