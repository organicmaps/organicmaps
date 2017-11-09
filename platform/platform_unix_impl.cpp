#include "platform/platform.hpp"
#include "platform/platform_unix_impl.hpp"

#include "coding/file_name_utils.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include "std/algorithm.hpp"
#include "std/cstring.hpp"
#include "std/regex.hpp"
#include "std/unique_ptr.hpp"

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(OMIM_OS_MAC) || defined(OMIM_OS_IPHONE)
  #include <sys/mount.h>
#else
  #include <sys/vfs.h>
#endif

namespace
{
struct CloseDir
{
  void operator()(DIR * dir) const
  {
    if (dir)
      closedir(dir);
  }
};
}  // namespace

void Platform::GetSystemFontNames(FilesList & res) const
{
#if defined(OMIM_OS_MAC) || defined(OMIM_OS_IPHONE)
#else
  char const * fontsWhitelist[] = {
    "Roboto-Medium.ttf",
    "Roboto-Regular.ttf",
    "DroidSansFallback.ttf",
    "DroidSansFallbackFull.ttf",
    "DroidSans.ttf",
    "DroidSansArabic.ttf",
    "DroidSansSemc.ttf",
    "DroidSansSemcCJK.ttf",
    "DroidNaskh-Regular.ttf",
    "Lohit-Bengali.ttf",
    "Lohit-Devanagari.ttf",
    "Lohit-Tamil.ttf",
    "PakType Naqsh.ttf",
    "wqy-microhei.ttc",
    "Jomolhari.ttf",
    "Jomolhari-alpha3c-0605331.ttf",
    "Padauk.ttf",
    "KhmerOS.ttf",
    "Umpush.ttf",
    "DroidSansThai.ttf",
    "DroidSansArmenian.ttf",
    "DroidSansEthiopic-Regular.ttf",
    "DroidSansGeorgian.ttf",
    "DroidSansHebrew-Regular.ttf",
    "DroidSansHebrew.ttf",
    "DroidSansJapanese.ttf",
    "LTe50872.ttf",
    "LTe50259.ttf",
    "DevanagariOTS.ttf",
    "FreeSans.ttf",
    "DejaVuSans.ttf",
    "arial.ttf",
    "AbyssinicaSIL-R.ttf",
  };

  char const * systemFontsPath[] = {
    "/system/fonts/",
#ifdef OMIM_OS_LINUX
    "/usr/share/fonts/truetype/roboto/",
    "/usr/share/fonts/truetype/droid/",
    "/usr/share/fonts/truetype/ttf-dejavu/",
    "/usr/share/fonts/truetype/wqy/",
    "/usr/share/fonts/truetype/freefont/",
    "/usr/share/fonts/truetype/padauk/",
    "/usr/share/fonts/truetype/dzongkha/",
    "/usr/share/fonts/truetype/ttf-khmeros-core/",
    "/usr/share/fonts/truetype/tlwg/",
    "/usr/share/fonts/truetype/abyssinica/",
    "/usr/share/fonts/truetype/paktype/",
    "/usr/share/fonts/truetype/mapsme/",
#endif
  };

  const uint64_t fontSizeBlacklist[] = {
    183560,   // Samsung Duos DroidSans
    7140172,  // Serif font without Emoji
    14416824  // Serif font with Emoji
  };

  uint64_t fileSize = 0;

  for (size_t i = 0; i < ARRAY_SIZE(fontsWhitelist); ++i)
  {
    for (size_t j = 0; j < ARRAY_SIZE(systemFontsPath); ++j)
    {
      string const path = string(systemFontsPath[j]) + fontsWhitelist[i];
      if (IsFileExistsByFullPath(path))
      {
        if (GetFileSizeByName(path, fileSize))
        {
          uint64_t const * end = fontSizeBlacklist + ARRAY_SIZE(fontSizeBlacklist);
          if (find(fontSizeBlacklist, end, fileSize) == end)
          {
            res.push_back(path);
            LOG(LINFO, ("Found usable system font", path, "with file size", fileSize));
          }
        }
      }
    }
  }
#endif
}

// static
Platform::EError Platform::RmDir(string const & dirName)
{
  if (rmdir(dirName.c_str()) != 0)
    return ErrnoToError();
  return ERR_OK;
}

// static
Platform::EError Platform::GetFileType(string const & path, EFileType & type)
{
  struct stat stats;
  if (stat(path.c_str(), &stats) != 0)
    return ErrnoToError();
  if (S_ISREG(stats.st_mode))
    type = FILE_TYPE_REGULAR;
  else if (S_ISDIR(stats.st_mode))
    type = FILE_TYPE_DIRECTORY;
  else
    type = FILE_TYPE_UNKNOWN;
  return ERR_OK;
}

// static
bool Platform::IsFileExistsByFullPath(string const & filePath)
{
  struct stat s;
  return stat(filePath.c_str(), &s) == 0;
}

//static
void Platform::DisableBackupForFile(string const & filePath) {}

// static
bool Platform::IsCustomTextureAllocatorSupported() { return true; }

bool Platform::IsDirectoryEmpty(string const & directory)
{
  unique_ptr<DIR, CloseDir> dir(opendir(directory.c_str()));
  if (!dir)
    return true;

  struct dirent * entry;

  // Invariant: all files met so far are "." or "..".
  while ((entry = readdir(dir.get())) != nullptr)
  {
    // A file is not a special UNIX file. Early exit here.
    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
      return false;
  }
  return true;
}

bool Platform::GetFileSizeByFullPath(string const & filePath, uint64_t & size)
{
  struct stat s;
  if (stat(filePath.c_str(), &s) == 0)
  {
    size = s.st_size;
    return true;
  }
  else return false;
}

Platform::TStorageStatus Platform::GetWritableStorageStatus(uint64_t neededSize) const
{
  struct statfs st;
  int const ret = statfs(m_writableDir.c_str(), &st);

  LOG(LDEBUG, ("statfs return =", ret,
               "; block size =", st.f_bsize,
               "; blocks available =", st.f_bavail));

  if (ret != 0)
  {
    LOG(LERROR, ("Path:", m_writableDir, "statfs error:", ErrnoToError()));
    return STORAGE_DISCONNECTED;
  }

  /// @todo May be add additional storage space.
  if (st.f_bsize * st.f_bavail < neededSize)
    return NOT_ENOUGH_SPACE;

  return STORAGE_OK;
}

uint64_t Platform::GetWritableStorageSpace() const
{
  struct statfs st;
  int const ret = statfs(m_writableDir.c_str(), &st);

  LOG(LDEBUG, ("statfs return =", ret,
               "; block size =", st.f_bsize,
               "; blocks available =", st.f_bavail));

  if (ret != 0)
    LOG(LERROR, ("Path:", m_writableDir, "statfs error:", ErrnoToError()));

  return (ret != 0) ? 0 : st.f_bsize * st.f_bavail;
}

namespace pl
{
void EnumerateFilesByRegExp(string const & directory, string const & regexp,
                            vector<string> & res)
{
  unique_ptr<DIR, CloseDir> dir(opendir(directory.c_str()));
  if (!dir)
    return;

  regex exp(regexp);

  struct dirent * entry;
  while ((entry = readdir(dir.get())) != 0)
  {
    string const name(entry->d_name);
    if (regex_search(name.begin(), name.end(), exp))
      res.push_back(name);
  }
}
}  // namespace pl
