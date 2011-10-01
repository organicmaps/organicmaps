#include "platform.hpp"

#include "../coding/file_reader.hpp"

#include "../std/target_os.hpp"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryFile>

// default writable directory name for dev/standalone installs
#define MAPDATA_DIR "data"
// default writable dir name in LocalAppData or ~/Library/AppData/ folders
#define LOCALAPPDATA_DIR "MapsWithMe"
// default Resources read-only dir
#define RESOURCES_DIR "Resources"

/// @name Platform-dependent implementations in separate files
//@{
bool GetUserWritableDir(string & outDir);
bool GetPathToBinary(string & outDir);
//@}

static bool IsDirectoryWritable(string const & dir)
{
  if (!dir.empty())
  {
    QString qDir = dir.c_str();
    if (dir[dir.size() - 1] != '/' && dir[dir.size() - 1] != '\\')
      qDir.append('/');

    QTemporaryFile file(qDir + "XXXXXX");
    if (file.open())
      return true;
  }
  return false;
}

/// Scans all upper directories for the presence of given directory
/// @param[in] startPath full path to lowest file in hierarchy (usually binary)
/// @param[in] dirName directory name we want to be present
/// @return if not empty, contains full path to existing directory
static string DirFinder(string const & startPath, string dirName)
{
  char const SLASH = QDir::separator().toAscii();
  dirName = SLASH + dirName + SLASH;

  size_t slashPos = startPath.size();
  while (slashPos > 0 && (slashPos = startPath.rfind(SLASH, slashPos - 1)) != string::npos)
  {
    string const dir = startPath.substr(0, slashPos) + dirName;
    if (QFileInfo(dir.c_str()).exists())
      return dir;
  }
  return string();
}

static bool GetOSSpecificResourcesDir(string const & exePath, string & dir)
{
  dir = DirFinder(exePath, RESOURCES_DIR);
  return !dir.empty();
}

static void InitResourcesDir(string & dir)
{
  // Resources dir can be any "data" folder found in the nearest upper directory,
  // where all necessary resources files are present and accessible
  string exePath;
  CHECK( GetPathToBinary(exePath), ("Can't get full path to executable") );
  dir = DirFinder(exePath, MAPDATA_DIR);
  if (dir.empty())
  {
    CHECK( GetOSSpecificResourcesDir(exePath, dir), ("Can't retrieve resources directory") );
  }

  /// @todo Check all necessary files
}

static void InitWritableDir(string & dir)
{
  // Writable dir can be any "data" folder found in the nearest upper directory
  // ./data     - For Windows portable builds
  // ../data
  // ../../data - For developer builds
  // etc. (for Mac in can be up to 6 levels above due to packages structure
  // and if no _writable_ "data" folder was found, User/Application Data/MapsWithMe will be used

  string path;
  CHECK( GetPathToBinary(path), ("Can't get full path to executable") );
  dir = DirFinder(path, MAPDATA_DIR);
  if (dir.empty() || !IsDirectoryWritable(dir))
  {
    CHECK( GetUserWritableDir(dir), ("Can't get User's Application Data writable directory") );
  }
}

static string ReadPathForFile(string const & writableDir,
    string const & resourcesDir, string const & file)
{
  string fullPath = writableDir + file;
  if (!GetPlatform().IsFileExists(fullPath))
  {
    fullPath = resourcesDir + file;
    if (!GetPlatform().IsFileExists(fullPath))
      MYTHROW(FileAbsentException, ("File doesn't exist", fullPath));
  }
  return fullPath;
}

////////////////////////////////////////////////////////////////////////////////////////
Platform::Platform()
{
  InitWritableDir(m_writableDir);
  InitResourcesDir(m_resourcesDir);
}

Platform::~Platform()
{
}

ModelReader * Platform::GetReader(string const & file) const
{
  return new FileReader(ReadPathForFile(m_writableDir, m_resourcesDir, file), 10, 12);
}

bool Platform::GetFileSize(string const & file, uint64_t & size) const
{
  QFileInfo f(file.c_str());
  size = static_cast<uint64_t>(f.size());
  return size != 0;
}

void Platform::GetFilesInDir(string const & directory, string const & mask, FilesList & outFiles) const
{
  outFiles.clear();
  QDir dir(directory.c_str(), mask.c_str(), QDir::Unsorted,
           QDir::Files | QDir::Readable | QDir::Dirs | QDir::NoDotAndDotDot);
  int const count = dir.count();
  for (int i = 0; i < count; ++i)
    outFiles.push_back(dir[i].toUtf8().data());
}

string Platform::DeviceName() const
{
  return OMIM_OS_NAME;
}

double Platform::VisualScale() const
{
  return 1.0;
}

string Platform::SkinName() const
{
  return "basic.skn";
}

void Platform::GetFontNames(FilesList & res) const
{
  GetFilesInDir(ResourcesDir(), "*.ttf", res);
  sort(res.begin(), res.end());
}

int Platform::MaxTilesCount() const
{
  return 120;
}

int Platform::TileSize() const
{
  return 256;
}

int Platform::ScaleEtalonSize() const
{
  return 512 + 256;
}

///////////////////////////////////////////////////////////////////////////////
extern "C" Platform & GetPlatform()
{
  static Platform platform;
  return platform;
}
