#include "platform/constants.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/platform.hpp"
#include "platform/platform_unix_impl.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

#include "coding/zip_reader.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "base/thread.hpp"

#include <algorithm>
#include <memory>
#include <ranges>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>  // for sysconf

Platform::Platform()
{
  /// @see initialization routine in android/sdk/src/main/cpp/com/.../Platform.hpp
}

#ifdef DEBUG
namespace
{
class DbgLogger
{
public:
  explicit DbgLogger(std::string const & file) : m_file(file) {}

  ~DbgLogger() { LOG(LDEBUG, ("Source for file", m_file, "is", m_src)); }

  void SetSource(char src) { m_src = src; }

private:
  std::string const & m_file;
  char m_src;
};
}  // namespace
#endif

std::unique_ptr<ModelReader> Platform::GetReader(std::string const & file, std::string searchScope) const
{
  std::string ext = base::GetFileExtension(file);
  strings::AsciiToLower(ext);
  ASSERT(!ext.empty(), ());

  uint32_t const logPageSize = (ext == DATA_FILE_EXTENSION) ? READER_CHUNK_LOG_SIZE : 10;
  uint32_t const logPageCount = (ext == DATA_FILE_EXTENSION) ? READER_CHUNK_LOG_COUNT : 4;

  if (searchScope.empty())
  {
    if (file[0] == '/')
      searchScope = "f";
    else
    {
      ASSERT(ext != ".kml" && ext != ".kmb" && ext != ".kmz", ("BookmarkManager is responsible for that"));

      if (ext == DATA_FILE_EXTENSION)
        if (file.starts_with(WORLD_COASTS_FILE_NAME) || file.starts_with(WORLD_FILE_NAME))
          searchScope = "wsr";
        else
          searchScope = "w";
      else if (file == SETTINGS_FILE_NAME)
        searchScope = "s";
      else
        searchScope = "rw";
    }
  }

#ifdef DEBUG
  DbgLogger logger(file);
#endif

  for (char const s : searchScope)
  {
#ifdef DEBUG
    logger.SetSource(s);
#endif

    switch (s)
    {
    case 'w':
    {
      auto const path = base::JoinPath(m_writableDir, file);
      if (IsFileExistsByFullPath(path))
        return std::make_unique<FileReader>(path, logPageSize, logPageCount);
      break;
    }

    case 's':
    {
      auto const path = base::JoinPath(m_settingsDir, file);
      if (IsFileExistsByFullPath(path))
        return std::make_unique<FileReader>(path, logPageSize, logPageCount);
      break;
    }

    case 'f':
      if (IsFileExistsByFullPath(file))
        return make_unique<FileReader>(file, logPageSize, logPageCount);
      break;

    case 'r':
      ASSERT_EQUAL(file.find("assets/"), std::string::npos, ());
      try
      {
        return make_unique<ZipFileReader>(m_resourcesDir, "assets/" + file, logPageSize, logPageCount);
      }
      catch (Reader::OpenException const & e)
      {
        LOG(LWARNING, ("Can't get reader:", e.what()));
      }
      break;

    default: CHECK(false, ("Unsupported source:", s)); break;
    }
  }

  LOG(LWARNING, ("Can't get reader for:", file, "in scope", searchScope));
  MYTHROW(FileAbsentException, ("File not found", file));
  return nullptr;
}

void Platform::GetFilesByRegExp(std::string const & directory, std::string const & regexp, FilesList & res)
{
  if (ZipFileReader::IsZip(directory))
  {
    // Get files list inside zip file
    typedef ZipFileReader::FileList FilesT;
    FilesT fList;
    ZipFileReader::FilesList(directory, fList);

    std::regex exp(regexp);

    for (FilesT::iterator it = fList.begin(); it != fList.end(); ++it)
    {
      std::string & name = it->first;
      if (std::regex_search(name.begin(), name.end(), exp))
      {
        // Remove assets/ prefix - clean files are needed for fonts white/blacklisting logic
        size_t const ASSETS_LENGTH = 7;
        if (name.find("assets/") == 0)
          name.erase(0, ASSETS_LENGTH);

        res.push_back(name);
      }
    }
  }
  else
    pl::EnumerateFilesByRegExp(directory, regexp, res);
}

int Platform::VideoMemoryLimit() const
{
  return 10 * 1024 * 1024;
}

int Platform::PreCachingDepth() const
{
  return 3;
}

bool Platform::GetFileSizeByName(std::string const & fileName, uint64_t & size) const
{
  try
  {
    size = ReaderPtr<Reader>(GetReader(fileName)).Size();
    return true;
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, ("Can't get file size for:", fileName));
    return false;
  }
}

// static
Platform::EError Platform::MkDir(std::string const & dirName)
{
  if (0 != mkdir(dirName.c_str(), 0755))
    return ErrnoToError();
  return Platform::ERR_OK;
}

void Platform::SetupMeasurementSystem() const
{
  auto units = measurement_utils::Units::Metric;
  if (settings::Get(settings::kMeasurementUnits, units))
    return;
  // @TODO Add correct implementation
  units = measurement_utils::Units::Metric;
  settings::Set(settings::kMeasurementUnits, units);
}

void Platform::GetSystemFontNames(FilesList & res) const
{
  bool wasRoboto = false;
  bool hasCJK = false;

  // Android 5–6 ship per-variant CJK files (one face each, ~1 MB metrics cache each). Collect
  // candidates during enumeration, then register exactly one — the best fallback for the user's
  // preferred variant. Android 7+ uses NotoSansCJK-Regular.ttc instead; that is handled in
  // GlyphManager via TTC face index selection.
  languages::CJKResolver const cjk;
  using CJKCandidate = std::pair<std::string, languages::CJKResolver::Variant>;
  std::vector<CJKCandidate> cjkCandidates;

  std::string const path = "/system/fonts/";
  pl::EnumerateFiles(path, [&](char const * entry)
  {
    std::string name(entry);
    if (name == "Roboto-Medium.ttf" || name == "Roboto-Regular.ttf")
    {
      wasRoboto = true;
      res.push_back(path + name);
      return;
    }

    if (auto const variant = languages::CJKResolver::FromFontFileName(name))
    {
      cjkCandidates.emplace_back(path + name, *variant);
      return;
    }

    if (!name.starts_with("NotoNaskh") && !name.starts_with("NotoSans"))
      return;

    if (name.find("-Regular") == std::string::npos)
      return;

    if (languages::CJKResolver::IsCJKContainerFileName(name))
      hasCJK = true;

    res.push_back(path + name);
  });

  // Android 5–6 devices ship at most one per-variant file, so the fallback only fires in
  // unusual configurations.
  for (auto const want : cjk.FallbackChain())
  {
    auto const it = std::ranges::find(cjkCandidates, want, &CJKCandidate::second);
    if (it != cjkCandidates.end())
    {
      res.push_back(it->first);
      hasCJK = true;
      break;
    }
  }

  if (!wasRoboto)
  {
    std::string droidSans = path + "DroidSans.ttf";
    if (IsFileExistsByFullPath(droidSans))
      res.push_back(std::move(droidSans));
  }

  // Legacy SC fallback for pre-Lollipop AOSP and vendor builds that ship neither the Pan-CJK TTC
  // nor any per-variant Noto file. Skip when a CJK font was already registered to avoid loading
  // a redundant ~1 MB glyph-metrics cache.
  if (!hasCJK)
  {
    std::string droidFallback = path + "DroidSansFallback.ttf";
    if (IsFileExistsByFullPath(droidFallback))
      res.push_back(std::move(droidFallback));
  }
}

// static
time_t Platform::GetFileCreationTime(std::string const & path)
{
  // Note: Android's plain stat() does not expose birth time. Using st_atim as an approximation
  // is a pre-existing behavior quirk; statx would require API >= 30 (current minSdk is 21).
  struct stat st;
  if (0 == stat(path.c_str(), &st))
    return st.st_atim.tv_sec;
  return 0;
}

// static
time_t Platform::GetFileModificationTime(std::string const & path)
{
  struct stat st;
  if (0 == stat(path.c_str(), &st))
    return st.st_mtim.tv_sec;
  return 0;
}

// static
bool Platform::SetFileModificationTime(std::string const & path, time_t modTime)
{
  struct timespec times[2] = {};
  times[0].tv_nsec = UTIME_OMIT;  // access time: unchanged
  times[1].tv_sec = modTime;      // modification time
  return utimensat(AT_FDCWD, path.c_str(), times, 0) == 0;
}
