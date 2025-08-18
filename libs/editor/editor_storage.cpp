#include "editor/editor_storage.hpp"

#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

#include "base/logging.hpp"

#include <string>

using namespace pugi;

namespace
{
char const * kEditorXMLFileName = "edits.xml";

std::string GetEditorFilePath()
{
  return GetPlatform().WritablePathForFile(kEditorXMLFileName);
}
}  // namespace

namespace editor
{
// StorageLocal ------------------------------------------------------------------------------------
bool LocalStorage::Save(xml_document const & doc)
{
  auto const editorFilePath = GetEditorFilePath();

  std::lock_guard<std::mutex> guard(m_mutex);

  return base::WriteToTempAndRenameToFile(editorFilePath, [&doc](std::string const & fileName)
  { return doc.save_file(fileName.data(), "  " /* indent */); });
}

bool LocalStorage::Load(xml_document & doc)
{
  auto const editorFilePath = GetEditorFilePath();

  std::lock_guard<std::mutex> guard(m_mutex);

  auto const result = doc.load_file(editorFilePath.c_str());
  // Note: status_file_not_found is ok if a user has never made any edits.
  if (result != status_ok && result != status_file_not_found)
  {
    LOG(LERROR, ("Can't load map edits from disk:", editorFilePath));
    return false;
  }

  return true;
}

bool LocalStorage::Reset()
{
  std::lock_guard<std::mutex> guard(m_mutex);

  return base::DeleteFileX(GetEditorFilePath());
}

// StorageMemory -----------------------------------------------------------------------------------
bool InMemoryStorage::Save(xml_document const & doc)
{
  m_doc.reset(doc);
  return true;
}

bool InMemoryStorage::Load(xml_document & doc)
{
  doc.reset(m_doc);
  return true;
}

bool InMemoryStorage::Reset()
{
  m_doc.reset();
  return true;
}
}  // namespace editor
