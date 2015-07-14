#pragma once
#include "testing/testing.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

//TODO(ldragunov, gorshenin) If there will be one more usage of this header, we must extract 
// it to the standalone test support library.

namespace platform
{
namespace tests
{
// Scoped test directory in a writable dir.
class ScopedTestDir
{
public:
  /// Creates test dir in a writable directory.
  /// @param path Path for a testing directory, should be relative to writable-dir.
  ScopedTestDir(string const & relativePath)
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
        CHECK(false, ("Can't create directory:", GetFullPath(), "error:", ret));
        break;
    }
  }

  ~ScopedTestDir()
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
        LOG(LWARNING, (fullPath, "was deleted before destruction of ScopedTestDir."));
        break;
      case Platform::ERR_DIRECTORY_NOT_EMPTY:
        LOG(LWARNING, ("There are files in", fullPath));
        break;
      default:
        LOG(LWARNING, ("Platform::RmDir() error for", fullPath, ":", ret));
        break;
    }
  }

  inline void Reset() { m_reset = true; }

  inline string const & GetFullPath() const { return m_fullPath; }

  inline string const & GetRelativePath() const { return m_relativePath; }

  bool Exists() const { return GetPlatform().IsFileExistsByFullPath(GetFullPath()); }

private:
  string const m_fullPath;
  string const m_relativePath;
  bool m_reset;

  DISALLOW_COPY_AND_MOVE(ScopedTestDir);
};

class ScopedTestFile
{
public:
  ScopedTestFile(string const & relativePath, string const & contents)
      : m_fullPath(my::JoinFoldersToPath(GetPlatform().WritableDir(), relativePath)), m_reset(false)
  {
    {
      FileWriter writer(GetFullPath());
      writer.Write(contents.data(), contents.size());
    }
    TEST(Exists(), ("Can't create test file", GetFullPath()));
  }

  ScopedTestFile(ScopedTestDir const & dir, CountryFile const & countryFile, TMapOptions file,
                 string const & contents)
      : ScopedTestFile(
            my::JoinFoldersToPath(dir.GetRelativePath(), countryFile.GetNameWithExt(file)),
            contents)
  {
  }

  ~ScopedTestFile()
  {
    if (m_reset)
      return;
    if (!Exists())
    {
      LOG(LWARNING, ("File", GetFullPath(), "was deleted before dtor of ScopedTestFile."));
      return;
    }
    if (!my::DeleteFileX(GetFullPath()))
      LOG(LWARNING, ("Can't remove test file:", GetFullPath()));
  }

  inline string const & GetFullPath() const { return m_fullPath; }

  inline void Reset() { m_reset = true; }

  bool Exists() const { return GetPlatform().IsFileExistsByFullPath(GetFullPath()); }

private:
  string const m_fullPath;
  bool m_reset;

  DISALLOW_COPY_AND_MOVE(ScopedTestFile);
};

string DebugPrint(ScopedTestDir const & dir)
{
  ostringstream os;
  os << "ScopedTestDir [" << dir.GetFullPath() << "]";
  return os.str();
}

string DebugPrint(ScopedTestFile const & file)
{
  ostringstream os;
  os << "ScopedTestFile [" << file.GetFullPath() << "]";
  return os.str();
}
}  // namespace tests
}  // namespace platform
