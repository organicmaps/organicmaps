#include "editor/editor_storage.hpp"

#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

#include "base/logging.hpp"

#include "std/string.hpp"

#include "3party/pugixml/src/pugixml.hpp"

using namespace pugi;

namespace
{
constexpr char const * kEditorXMLFileName = "edits.xml";
string GetEditorFilePath() { return GetPlatform().WritablePathForFile(kEditorXMLFileName); }
}  // namespace

namespace editor
{
bool StorageLocal::Save(xml_document const & doc)
{
  auto const editorFilePath = GetEditorFilePath();
  return my::WriteToTempAndRenameToFile(editorFilePath, [&doc](string const & fileName) {
    return doc.save_file(fileName.data(), "  ");
  });
}

bool StorageLocal::Load(xml_document & doc)
{
  auto const editorFilePath = GetEditorFilePath();
  auto const result = doc.load_file(editorFilePath.c_str());
  // Note: status_file_not_found is ok if user has never made any edits.
  if (result != status_ok && result != status_file_not_found)
  {
    LOG(LERROR, ("Can't load map edits from disk:", editorFilePath));
    return false;
  }

  return true;
}

void StorageLocal::Reset()
{
  my::DeleteFileX(GetEditorFilePath());
}


StorageMemory::StorageMemory()
  : m_doc(make_unique <xml_document> ())
{}

bool StorageMemory::Save(xml_document const & doc)
{
  m_doc->reset(doc);
  return true;
}

bool StorageMemory::Load(xml_document & doc)
{
  doc.reset(*m_doc);
  return true;
}

void StorageMemory::Reset()
{
  m_doc->reset();
}
}  // namespace editor
