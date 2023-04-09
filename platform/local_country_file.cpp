#include "platform/local_country_file.hpp"

#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/sha1.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <sstream>

namespace platform
{
using namespace std;

LocalCountryFile::LocalCountryFile() : m_version(0) {}

LocalCountryFile::LocalCountryFile(string directory, CountryFile countryFile, int64_t version)
: m_directory(std::move(directory)), m_countryFile(std::move(countryFile)), m_version(version)
{
}

void LocalCountryFile::SyncWithDisk()
{
  // World files from resources have an empty directory. See todo in the header.
  if (m_directory.empty())
    return;

  m_files = {};
  uint64_t size = 0;

  // Now we are not working with several files at the same time and diffs have greater priority.
  for (MapFileType type : {MapFileType::Diff, MapFileType::Map})
  {
    ASSERT_LESS(base::Underlying(type), m_files.size(), ());

    if (GetPlatform().GetFileSizeByFullPath(GetPath(type), size))
    {
      m_files[base::Underlying(type)] = size;
      break;
    }
  }
}

void LocalCountryFile::DeleteFromDisk(MapFileType type) const
{
  ASSERT_LESS(base::Underlying(type), m_files.size(), ());

  if (!OnDisk(type))
    return;

  if (!base::DeleteFileX(GetPath(type)))
    LOG(LERROR, (type, "from", *this, "wasn't deleted from disk."));
}

string LocalCountryFile::GetPath(MapFileType type) const
{
  return base::JoinPath(m_directory, GetFileName(type));
}

std::string LocalCountryFile::GetFileName(MapFileType type) const
{
  return m_countryFile.GetFileName(type);
}

uint64_t LocalCountryFile::GetSize(MapFileType type) const
{
  auto const ut = base::Underlying(type);
  ASSERT_LESS(ut, m_files.size(), ());
  return m_files[ut].value_or(0);
}

bool LocalCountryFile::HasFiles() const
{
  return std::any_of(m_files.cbegin(), m_files.cend(),
                     [](auto value) { return value.has_value(); });
}

bool LocalCountryFile::OnDisk(MapFileType type) const
{
  auto const ut = base::Underlying(type);
  ASSERT_LESS(ut, m_files.size(), ());
  return m_files[ut].has_value();
}

bool LocalCountryFile::operator<(LocalCountryFile const & rhs) const
{
  if (m_countryFile != rhs.m_countryFile)
    return m_countryFile < rhs.m_countryFile;
  if (m_version != rhs.m_version)
    return m_version < rhs.m_version;
  if (m_directory != rhs.m_directory)
    return m_directory < rhs.m_directory;
  if (m_files != rhs.m_files)
    return m_files < rhs.m_files;
  return false;
}

bool LocalCountryFile::operator==(LocalCountryFile const & rhs) const
{
  return m_directory == rhs.m_directory && m_countryFile == rhs.m_countryFile &&
         m_version == rhs.m_version && m_files == rhs.m_files;
}

bool LocalCountryFile::ValidateIntegrity() const
{
  auto calculatedSha1 = coding::SHA1::CalculateBase64(GetPath(MapFileType::Map));
  ASSERT_EQUAL(calculatedSha1, m_countryFile.GetSha1(), ("Integrity failure"));
  return calculatedSha1 == m_countryFile.GetSha1();
}

// static
LocalCountryFile LocalCountryFile::MakeForTesting(string countryFileName, int64_t version)
{
  LocalCountryFile localFile(GetPlatform().WritableDir(), CountryFile(std::move(countryFileName)), version);
  localFile.SyncWithDisk();
  return localFile;
}

// static
LocalCountryFile LocalCountryFile::MakeTemporary(string const & fullPath)
{
  string name = fullPath;
  base::GetNameFromFullPath(name);
  base::GetNameWithoutExt(name);

  return LocalCountryFile(base::GetDirectory(fullPath), CountryFile(std::move(name)), 0 /* version */);
}

string DebugPrint(LocalCountryFile const & file)
{
  ostringstream filesStream;
  filesStream << "[";
  bool fileAdded = false;
  for (auto const & mapFile : file.m_files)
  {
    if (mapFile)
    {
      filesStream << (fileAdded ? ", " : "") << *mapFile;
      fileAdded = true;
    }
  }
  filesStream << "]";

  ostringstream os;
  os << "LocalCountryFile [" << file.m_directory << ", " << DebugPrint(file.m_countryFile) << ", "
     << file.m_version << ", " << filesStream.str() << "]";
  return os.str();
}
}  // namespace platform
