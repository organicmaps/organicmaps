#include "platform/platform_tests_support/scoped_file.hpp"

#include "testing/testing.hpp"

#include "platform/country_file.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "base/logging.hpp"

#include "std/sstream.hpp"

namespace platform
{
namespace tests_support
{
ScopedFile::ScopedFile(string const & relativePath, string const & contents)
    : m_fullPath(my::JoinFoldersToPath(GetPlatform().WritableDir(), relativePath)), m_reset(false)
{
  {
    FileWriter writer(GetFullPath());
    writer.Write(contents.data(), contents.size());
  }
  TEST(Exists(), ("Can't create test file", GetFullPath()));
}

ScopedFile::ScopedFile(ScopedDir const & dir, CountryFile const & countryFile, MapOptions file,
                       string const & contents)
    : ScopedFile(my::JoinFoldersToPath(dir.GetRelativePath(), countryFile.GetNameWithExt(file)),
                 contents)
{
}

ScopedFile::~ScopedFile()
{
  if (m_reset)
    return;
  if (!Exists())
  {
    LOG(LWARNING, ("File", GetFullPath(), "was deleted before dtor of ScopedFile."));
    return;
  }
  if (!my::DeleteFileX(GetFullPath()))
    LOG(LWARNING, ("Can't remove test file:", GetFullPath()));
}

string DebugPrint(ScopedFile const & file)
{
  ostringstream os;
  os << "ScopedFile [" << file.GetFullPath() << "]";
  return os.str();
}
}  // namespace tests_support
}  // namespace platform
