#include "platform/platform_tests_support/scoped_dir.hpp"

#include "testing/testing.hpp"

#include "coding/file_name_utils.hpp"

#include "base/logging.hpp"

#include "std/sstream.hpp"

namespace platform
{
namespace tests_support
{
ScopedDir::ScopedDir(string const & relativePath)
    : m_fullPath(my::JoinFoldersToPath(GetPlatform().WritableDir(), relativePath)),
      m_relativePath(relativePath),
      m_reset(false)
{
  Platform & platform = GetPlatform();
  Platform::EError ret = platform.MkDir(GetFullPath());
  switch (ret)
  {
    case Platform::ERR_OK:
      break;
    case Platform::ERR_FILE_ALREADY_EXISTS:
      Platform::EFileType type;
      TEST_EQUAL(Platform::ERR_OK, Platform::GetFileType(GetFullPath(), type), ());
      TEST_EQUAL(Platform::FILE_TYPE_DIRECTORY, type, ());
      break;
    default:
      TEST(false, ("Can't create directory:", GetFullPath(), "error:", ret));
      break;
  }
}

ScopedDir::~ScopedDir()
{
  if (m_reset)
    return;

  string const fullPath = GetFullPath();
  Platform::EError ret = Platform::RmDir(fullPath);
  switch (ret)
  {
    case Platform::ERR_OK:
      break;
    case Platform::ERR_FILE_DOES_NOT_EXIST:
      LOG(LWARNING, (fullPath, "was deleted before destruction of ScopedDir."));
      break;
    case Platform::ERR_DIRECTORY_NOT_EMPTY:
      LOG(LWARNING, ("There are files in", fullPath));
      break;
    default:
      LOG(LWARNING, ("Platform::RmDir() error for", fullPath, ":", ret));
      break;
  }
}

string DebugPrint(ScopedDir const & dir)
{
  ostringstream os;
  os << "ScopedDir [" << dir.GetFullPath() << "]";
  return os.str();
}
}  // namespace tests_support
}  // namespace platform
