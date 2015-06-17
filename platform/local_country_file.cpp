#include "platform/local_country_file.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/file_name_utils.hpp"
#include "platform/platform.hpp"

#include "base/logging.hpp"

#include "std/sstream.hpp"

namespace platform
{
LocalCountryFile::LocalCountryFile()
    : m_version(0), m_files(TMapOptions::ENothing), m_mapSize(0), m_routingSize()
{
}

LocalCountryFile::LocalCountryFile(string const & directory, CountryFile const & countryFile,
                                   int64_t version)
    : m_directory(directory),
      m_countryFile(countryFile),
      m_version(version),
      m_files(TMapOptions::ENothing),
      m_mapSize(0),
      m_routingSize(0)
{
}

void LocalCountryFile::SyncWithDisk()
{
  m_files = TMapOptions::ENothing;
  m_mapSize = 0;
  m_routingSize = 0;

  Platform & platform = GetPlatform();

  if (platform.GetFileSizeByFullPath(GetPath(TMapOptions::EMap), m_mapSize))
    m_files = SetOptions(m_files, TMapOptions::EMap);

  string const routingPath = GetPath(TMapOptions::ECarRouting);
  if (platform.GetFileSizeByFullPath(routingPath, m_routingSize))
    m_files = SetOptions(m_files, TMapOptions::ECarRouting);
}

void LocalCountryFile::DeleteFromDisk(TMapOptions files) const
{
  for (TMapOptions file : {TMapOptions::EMap, TMapOptions::ECarRouting})
  {
    if (OnDisk(file) && HasOptions(files, file))
    {
      if (!my::DeleteFileX(GetPath(file)))
        LOG(LERROR, (file, "from", *this, "wasn't deleted from disk."));
    }
  }
}

string LocalCountryFile::GetPath(TMapOptions file) const
{
  return my::JoinFoldersToPath(m_directory, m_countryFile.GetNameWithExt(file));
}

uint32_t LocalCountryFile::GetSize(TMapOptions filesMask) const
{
  uint64_t size64 = 0;
  if (HasOptions(filesMask, TMapOptions::EMap))
    size64 += m_mapSize;
  if (HasOptions(filesMask, TMapOptions::ECarRouting))
    size64 += m_routingSize;
  uint32_t const size32 = static_cast<uint32_t>(size64);
  ASSERT_EQUAL(size32, size64, ());
  return size32;
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

// static
LocalCountryFile LocalCountryFile::MakeForTesting(string const & countryFileName)
{
  CountryFile const countryFile(countryFileName);
  LocalCountryFile localFile(GetPlatform().WritableDir(), countryFile, 0 /* version */);
  localFile.SyncWithDisk();
  return localFile;
}

string DebugPrint(LocalCountryFile const & file)
{
  ostringstream os;
  os << "LocalCountryFile [" << file.m_directory << ", " << DebugPrint(file.m_countryFile) << ", "
     << file.m_version << ", " << DebugPrint(file.m_files) << "]";
  return os.str();
}
}  // namespace platform
