#include "editor/editor_notes.hpp"

#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

#include "base/string_utils.hpp"
#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/timer.hpp"

#include "3party/pugixml/src/pugixml.hpp"

namespace
{
bool LoadFromXml(pugi::xml_document const & xml,
                 vector<editor::Note> & notes,
                 uint32_t & uploadedNotesCount)
{
  uint64_t notesCount;
  auto const root = xml.child("notes");
  if (!strings::to_uint64(root.attribute("count").value(), notesCount))
    return false;
  uploadedNotesCount = static_cast<uint32_t>(notesCount);
  for (auto const xNode : root.select_nodes("note"))
  {
    m2::PointD point;

    auto const node = xNode.node();
    auto const point_x = node.attribute("x");
    if (!point_x || !strings::to_double(point_x.value(), point.x))
      return false;

    auto const point_y = node.attribute("y");
    if (!point_y || !strings::to_double(point_y.value(), point.y))
      return false;

    auto const text = node.attribute("text");
    if (!text)
      return false;

    notes.emplace_back(point, text.value());
  }
  return true;
}

void SaveToXml(vector<editor::Note> const & notes,
               pugi::xml_document & xml,
               uint32_t const UploadedNotesCount)
{
  auto root = xml.append_child("notes");
  root.append_attribute("count") = UploadedNotesCount;
  for (auto const & note : notes)
  {
    auto node = root.append_child("note");
    node.append_attribute("x") = DebugPrint(note.m_point.x).data();
    node.append_attribute("y") = DebugPrint(note.m_point.y).data();
    node.append_attribute("text") = note.m_note.data();
  }
}
}  // namespace

namespace editor
{
Notes::Notes(string const & fileName)
    : m_fileName(fileName)
{
  Load();
}

void Notes::CreateNote(m2::PointD const & point, string const & text)
{
  lock_guard<mutex> g(m_mu);
  m_notes.emplace_back(point, text);
  Save();
}

void Notes::Upload()
{
  throw "NotImplemented";
}

bool Notes::Load()
{
  string content;
  try
  {
    auto const reader = GetPlatform().GetReader(m_fileName);
    reader->ReadAsString(content);
  }
  catch (FileAbsentException const &)
  {
    LOG(LINFO, ("No edits file."));
    return true;
  }
  catch (Reader::Exception const &)
  {
    LOG(LERROR, ("Can't process file.", m_fileName));
    return false;
  }

  pugi::xml_document xml;
  if (!xml.load_buffer(content.data(), content.size()))
  {
    LOG(LERROR, ("Can't load notes, xml is illformed."));
    return false;
  }

  lock_guard<mutex> g(m_mu);
  m_notes.clear();
  if (!LoadFromXml(xml, m_notes, m_uploadedNotes))
  {
    LOG(LERROR, ("Can't load notes, file is illformed."));
    return false;
  }

  return true;
}

/// Not thread-safe, use syncronization.
bool Notes::Save()
{
  pugi::xml_document xml;
  SaveToXml(m_notes, xml, m_uploadedNotes);

  string const tmpFileName = m_fileName + ".tmp";
  if (!xml.save_file(tmpFileName.data(), "  "))
  {
    LOG(LERROR, ("Can't save map edits into", tmpFileName));
    return false;
  }
  else if (!my::RenameFileX(tmpFileName, m_fileName))
  {
    LOG(LERROR, ("Can't rename file", tmpFileName, "to", m_fileName));
    return false;
  }
  return true;
}
}
