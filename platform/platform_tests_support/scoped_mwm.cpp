#include "scoped_mwm.hpp"

#include "defines.hpp"

#include "indexer/data_header.hpp"

#include "platform/mwm_version.hpp"

#include "testing/testing.hpp"

#include "coding/file_writer.hpp"
#include "coding/file_container.hpp"
#include "coding/internal/file_data.hpp"

using feature::DataHeader;
namespace platform
{
namespace tests_support
{
ScopedMwm::ScopedMwm(string const & fullPath) : m_fullPath(fullPath), m_reset(false)
{
  {
    DataHeader header;
    {
      FilesContainerW container(GetFullPath());

      //Each writer must be in it's own scope to avoid conflicts on the final write.
      {
        FileWriter versionWriter =container.GetWriter(VERSION_FILE_TAG);
        version::WriteVersion(versionWriter);
      }
      {
        FileWriter w = container.GetWriter(HEADER_FILE_TAG);
        header.Save(w);
      }
    }
  }
  TEST(Exists(), ("Can't create test file", GetFullPath()));
}

ScopedMwm::~ScopedMwm()
{
  if (m_reset)
    return;
  if (!Exists())
  {
    LOG(LWARNING, ("File", GetFullPath(), "was deleted before dtor of ScopedMwm."));
    return;
  }
  if (!my::DeleteFileX(GetFullPath()))
    LOG(LWARNING, ("Can't remove test file:", GetFullPath()));
}
}  // namespace tests_support
}  // namespace platfotm
