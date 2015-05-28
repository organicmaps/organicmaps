#include "platform/local_country_file.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/file_name_utils.hpp"
#include "platform/platform.hpp"

#include "base/logging.hpp"

#include "std/sstream.hpp"

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

  string const mapPath = GetPath(TMapOptions::EMapOnly);
  if (platform.GetFileSizeByName(mapPath, m_mapSize))
    m_files = m_files | TMapOptions::EMapOnly;

  string const routingPath = GetPath(TMapOptions::ECarRouting);
  if (platform.GetFileSizeByName(routingPath, m_routingSize))
    m_files = m_files | TMapOptions::ECarRouting;
}

void LocalCountryFile::DeleteFromDisk()
{
  for (TMapOptions file : {TMapOptions::EMapOnly, TMapOptions::ECarRouting})
  {
    if (OnDisk(file))
      my::DeleteFileX(GetPath(file));
  }
}

string LocalCountryFile::GetPath(TMapOptions file) const
{
  return my::JoinFoldersToPath(m_directory, m_countryFile.GetNameWithExt(file));
}

uint32_t LocalCountryFile::GetSize(TMapOptions filesMask) const
{
  uint64_t size64 = 0;
  if (filesMask & TMapOptions::EMapOnly)
    size64 += m_mapSize;
  if (filesMask & TMapOptions::ECarRouting)
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
  return m_files < rhs.m_files;
}

bool LocalCountryFile::operator==(LocalCountryFile const & rhs) const
{
  return m_countryFile == rhs.m_countryFile && m_version == rhs.m_version && m_files == rhs.m_files;
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
