#include "platform/platform_tests_support/scoped_dir.hpp"

#include "testing/testing.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <sstream>

namespace platform
{
namespace tests_support
{
ScopedDir::ScopedDir(std::string const & relativePath)
  : m_fullPath(base::JoinPath(GetPlatform().WritableDir(), relativePath))
  , m_relativePath(relativePath)
  , m_reset(false)
{
  Platform::EError ret = Platform::MkDir(GetFullPath());
  switch (ret)
  {
  case Platform::ERR_OK: break;
  case Platform::ERR_FILE_ALREADY_EXISTS:
    Platform::EFileType type;
    TEST_EQUAL(Platform::ERR_OK, Platform::GetFileType(GetFullPath(), type), ());
    TEST_EQUAL(Platform::EFileType::Directory, type, ());
    break;
  default: TEST(false, ("Can't create directory:", GetFullPath(), "error:", ret)); break;
  }
}

ScopedDir::ScopedDir(ScopedDir const & parent, std::string const & name)
  : ScopedDir(base::JoinPath(parent.GetRelativePath(), name))
{}

ScopedDir::~ScopedDir()
{
  if (m_reset)
    return;

  std::string const fullPath = GetFullPath();
  Platform::EError ret = Platform::RmDir(fullPath);
  switch (ret)
  {
  case Platform::ERR_OK: break;
  case Platform::ERR_FILE_DOES_NOT_EXIST:
    LOG(LERROR, (fullPath, "was deleted before destruction of ScopedDir."));
    break;
  case Platform::ERR_DIRECTORY_NOT_EMPTY: LOG(LERROR, ("There are files in", fullPath)); break;
  default: LOG(LERROR, ("Platform::RmDir() error for", fullPath, ":", ret)); break;
  }
}

std::string DebugPrint(ScopedDir const & dir)
{
  std::ostringstream os;
  os << "ScopedDir [" << dir.GetFullPath() << "]";
  return os.str();
}

ScopedDirCleanup::ScopedDirCleanup(std::string const & path) : m_fullPath(path)
{
  UNUSED_VALUE(Platform::MkDir(m_fullPath));
}

ScopedDirCleanup::~ScopedDirCleanup()
{
  UNUSED_VALUE(Platform::RmDirRecursively(m_fullPath));
}

}  // namespace tests_support
}  // namespace platform
