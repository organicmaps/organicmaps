#include "platform/local_country_file.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/sha1.hpp"

#include "base/logging.hpp"

#include "std/sstream.hpp"


namespace platform
{
LocalCountryFile::LocalCountryFile()
    : m_version(0), m_files(MapOptions::Nothing), m_mapSize(0), m_routingSize(0)
{
}

LocalCountryFile::LocalCountryFile(string const & directory, CountryFile const & countryFile,
                                   int64_t version)
    : m_directory(directory),
      m_countryFile(countryFile),
      m_version(version),
      m_files(MapOptions::Nothing),
      m_mapSize(0),
      m_routingSize(0)
{
}

void LocalCountryFile::SyncWithDisk()
{
  m_files = MapOptions::Nothing;
  m_mapSize = 0;
  m_routingSize = 0;
  Platform & platform = GetPlatform();

  if (platform.GetFileSizeByFullPath(GetPath(MapOptions::Diff), m_mapSize))
  {
    m_files = SetOptions(m_files, MapOptions::Diff);
    return;
  }

  if (platform.GetFileSizeByFullPath(GetPath(MapOptions::Map), m_mapSize))
    m_files = SetOptions(m_files, MapOptions::Map);

  if (version::IsSingleMwm(GetVersion()))
  {
    if (m_files == MapOptions::Map)
      m_files = SetOptions(m_files, MapOptions::CarRouting);
    return;
  }

  string const routingPath = GetPath(MapOptions::CarRouting);
  if (platform.GetFileSizeByFullPath(routingPath, m_routingSize))
    m_files = SetOptions(m_files, MapOptions::CarRouting);
}

void LocalCountryFile::DeleteFromDisk(MapOptions files) const
{
  vector<MapOptions> const mapOptions =
      version::IsSingleMwm(GetVersion()) ? vector<MapOptions>({MapOptions::Map, MapOptions::Diff})
                                         : vector<MapOptions>({MapOptions::Map, MapOptions::CarRouting});
  for (MapOptions file : mapOptions)
  {
    if (OnDisk(file) && HasOptions(files, file))
    {
      if (!base::DeleteFileX(GetPath(file)))
        LOG(LERROR, (file, "from", *this, "wasn't deleted from disk."));
    }
  }
}

string LocalCountryFile::GetPath(MapOptions file) const
{
  return base::JoinFoldersToPath(m_directory, GetFileName(m_countryFile.GetName(), file, GetVersion()));
}

uint64_t LocalCountryFile::GetSize(MapOptions filesMask) const
{
  uint64_t size = 0;
  if (HasOptions(filesMask, MapOptions::Map))
    size += m_mapSize;
  if (!version::IsSingleMwm(GetVersion()) && HasOptions(filesMask, MapOptions::CarRouting))
    size += m_routingSize;

  return size;
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
  auto calculatedSha1 = coding::SHA1::CalculateBase64(GetPath(MapOptions::Map));
  ASSERT_EQUAL(calculatedSha1, m_countryFile.GetSha1(), ("Integrity failure"));
  return calculatedSha1 == m_countryFile.GetSha1();
}

// static
LocalCountryFile LocalCountryFile::MakeForTesting(string const & countryFileName, int64_t version)
{
  CountryFile const countryFile(countryFileName);
  LocalCountryFile localFile(GetPlatform().WritableDir(), countryFile, version);
  localFile.SyncWithDisk();
  return localFile;
}

// static
LocalCountryFile LocalCountryFile::MakeTemporary(string const & fullPath)
{
  string name = fullPath;
  base::GetNameFromFullPath(name);
  base::GetNameWithoutExt(name);

  return LocalCountryFile(base::GetDirectory(fullPath), CountryFile(name), 0 /* version */);
}


string DebugPrint(LocalCountryFile const & file)
{
  ostringstream os;
  os << "LocalCountryFile [" << file.m_directory << ", " << DebugPrint(file.m_countryFile) << ", "
     << file.m_version << ", " << DebugPrint(file.m_files) << "]";
  return os.str();
}
}  // namespace platform
