#include "map/bookmarks_sync/local_store.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/bookmark_manager.hpp"
#include "map/bookmarks_sync/content_hash.hpp"

#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/reader.hpp"

#include "kml/type_utils.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <thread>

namespace bookmarks_sync
{
namespace
{
// Core can stay busy with a long load. A task that hasn't started by then is dropped, a started one is awaited.
auto constexpr kStartTimeout = std::chrono::seconds(10);

/// Runs |task| on |thread| and waits for its result. Nothing if the task hasn't started within kStartTimeout, like
/// while core tears down; the task never runs then.
template <typename T>
std::optional<T> RunAndWait(Platform::Thread thread, std::function<T()> task)
{
  struct State
  {
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_started = false;
    bool m_abandoned = false;
    bool m_done = false;
    std::optional<T> m_result;
    std::exception_ptr m_error;
  };
  auto state = std::make_shared<State>();
  GetPlatform().RunTask(thread, [state, task = std::move(task)]()
  {
    {
      std::lock_guard lock(state->m_mutex);
      if (state->m_abandoned)
        return;
      state->m_started = true;
    }
    std::optional<T> result;
    std::exception_ptr error;
    try
    {
      result = task();
    }
    catch (...)
    {
      error = std::current_exception();
    }
    {
      std::lock_guard lock(state->m_mutex);
      state->m_result = std::move(result);
      state->m_error = error;
      state->m_done = true;
    }
    state->m_cv.notify_all();
  });

  std::unique_lock lock(state->m_mutex);
  if (!state->m_cv.wait_for(lock, kStartTimeout, [&state] { return state->m_started; }))
  {
    state->m_abandoned = true;
    return {};
  }
  state->m_cv.wait(lock, [&state] { return state->m_done; });
  if (state->m_error)
    std::rethrow_exception(state->m_error);
  return std::move(state->m_result);
}

// Core ignores "<name>.kml.tmp<thread>" files, so the replacement is atomic for it.
bool CopyAtomically(std::string const & from, std::string const & to)
{
  return base::WriteToTempAndRenameToFile(to, [&from](std::string const & tmp) { return base::CopyFileX(from, tmp); });
}

/// The local version with its category renamed, written next to it before the incoming version replaces it.
void WriteConflictCopy(std::string const & path, std::string const & copyPath, std::string const & device)
{
  if (auto data = LoadKmlFile(path, FileType::Kml))
  {
    auto & name = data->m_categoryData.m_name;
    auto const sourceName = kml::GetDefaultStr(name);
    kml::SetDefaultStr(
        name, (sourceName.empty() ? base::GetNameFromFullPathWithoutExt(path) : sourceName) + " (" + device + ")");
    if (!SaveKmlFileSafe(*data, copyPath, FileType::Kml))
      MYTHROW(RootException, ("Can't save the conflict copy", copyPath));
  }
  else if (!CopyAtomically(path, copyPath))
  {
    MYTHROW(RootException, ("Can't copy", path, "to", copyPath));
  }
}
}  // namespace

StagedFile::~StagedFile()
{
  base::DeleteFileX(m_path);
}

std::string DebugPrint(ApplyResult result)
{
  switch (result)
  {
  case ApplyResult::Applied: return "Applied";
  case ApplyResult::Ignored: return "Ignored";
  case ApplyResult::Changed: return "Changed";
  case ApplyResult::Deferred: return "Deferred";
  }
  UNREACHABLE();
}

LocalStore::LocalStore(BookmarkManager & manager, std::string dir, std::string stagingDir, HashAlgorithm algorithm)
  : m_manager(manager)
  , m_dir(std::move(dir))
  , m_stagingDir(std::move(stagingDir))
  , m_algorithm(algorithm)
{}

// static
void LocalStore::CleanStaging(std::string const & stagingDir)
{
  Platform::RmDirRecursively(stagingDir);
}

std::vector<LocalFile> LocalStore::Snapshot(bool requireDir) const
{
  std::vector<LocalFile> files;
  for (auto & name : ListNames())
  {
    // Deleted after it was listed: the next pass sees the deletion.
    try
    {
      auto hash = HashFile(GetPath(name), m_algorithm);
      files.push_back({std::move(name), std::move(hash)});
    }
    catch (Reader::Exception const & e)
    {
      LOG(LINFO, ("Skipping", name, e.Msg()));
    }
  }
  // A directory missing after lists were synced must not look empty: that would trash every list in the cloud.
  if (files.empty() && requireDir && !Platform::IsFileExistsByFullPath(m_dir))
    MYTHROW(RootException, (m_dir, "is missing"));
  return files;
}

std::vector<std::string> LocalStore::ListNames() const
{
  if (!RunAndWait<bool>(Platform::Thread::File, [] { return true; }))
    MYTHROW1(CloudException, SyncError::Transient, ("Core is busy saving bookmarks"));

  // Core writes "<name>.kml.tmp<thread>" files and renames them, so those never match.
  Platform::FilesList names;
  Platform::GetFilesByExt(m_dir, kKmlExtension, names);
  std::erase_if(names, [this](std::string const & name) { return Platform::IsDirectory(GetPath(name)); });
  std::sort(names.begin(), names.end());
  return names;
}

bool LocalStore::CanApplyChanges(CanApply const & canApply) const
{
  // Core loads files one at a time, also the copies that this pass adds: wait for them instead of deferring.
  auto const deadline = std::chrono::steady_clock::now() + kStartTimeout;
  while (std::chrono::steady_clock::now() < deadline)
  {
    auto const isLoading =
        RunAndWait<bool>(Platform::Thread::Gui, [this] { return m_manager.IsAsyncLoadingInProgress(); });
    if (!isLoading || !*isLoading)
      break;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return RunAndWait<bool>(Platform::Thread::Gui, [this, &canApply] { return IsAdmitted(canApply); }).value_or(false);
}

StagedFilePtr LocalStore::NewStagedFile() const
{
  static std::atomic<uint64_t> counter = 0;
  if (!Platform::MkDirChecked(m_stagingDir))
    MYTHROW(RootException, ("Can't create", m_stagingDir));
  // The extension selects the parser in core.
  return std::make_shared<StagedFile>(
      base::JoinPath(m_stagingDir, std::to_string(++counter) + std::string{kKmlExtension}));
}

StagedFilePtr LocalStore::Stage(std::string const & name) const
{
  auto staged = NewStagedFile();
  auto const copied = RunAndWait<bool>(Platform::Thread::File, [this, &name, &staged]
  {
    auto const path = GetPath(name);
    if (!Platform::IsFileExistsByFullPath(path))
      return false;
    if (!base::CopyFileX(path, staged->GetPath()))
      MYTHROW(RootException, ("Can't copy", path));
    return true;
  });
  if (!copied)
    MYTHROW1(CloudException, SyncError::Transient, ("Core is busy saving bookmarks"));
  return *copied ? staged : nullptr;
}

ApplyResult LocalStore::Install(std::string const & name, StagedFilePtr const & incoming,
                                std::optional<std::string> const & expectedHash, CanApply const & canApply) const
{
  return Replace(name, incoming, expectedHash, nullptr /* device */, {}, canApply);
}

ApplyResult LocalStore::ResolveConflict(std::string const & name, StagedFilePtr const & incoming,
                                        std::string const & expectedHash, std::string const & device,
                                        std::set<std::string> const & reserved, CanApply const & canApply) const
{
  return Replace(name, incoming, expectedHash, &device, reserved, canApply);
}

ApplyResult LocalStore::Replace(std::string const & name, StagedFilePtr const & incoming,
                                std::optional<std::string> const & expectedHash, std::string const * device,
                                std::set<std::string> const & reserved, CanApply const & canApply) const
{
  if (!LoadKmlFile(incoming->GetPath(), FileType::Kml))
  {
    LOG(LWARNING, ("Can't parse the cloud version of", name));
    return ApplyResult::Ignored;
  }

  auto const path = GetPath(name);
  struct Prepared
  {
    BookmarkManager::FileUpdatePtr m_update;
    std::string m_copyPath;
    bool m_admitted = false;
  };
  auto const prepared = RunAndWait<Prepared>(Platform::Thread::Gui, [&]
  {
    Prepared result;
    result.m_admitted = IsAdmitted(canApply);
    if (!result.m_admitted)
      return result;
    result.m_update = m_manager.GetFileUpdate(path);
    if (!result.m_update)
      return result;
    result.m_update->SetApplyCondition(canApply);
    if (device)
      result.m_copyPath = ChooseFreePath(base::GetNameFromFullPathWithoutExt(name) + " (" + *device + ")", reserved);
    return result;
  });
  if (!prepared || !prepared->m_admitted)
    return ApplyResult::Deferred;
  // Saves are pending: the file is about to change.
  if (!prepared->m_update)
    return ApplyResult::Changed;
  auto const & update = prepared->m_update;
  auto const & copyPath = prepared->m_copyPath;

  // The File thread orders the write after queued saves and imports, also for a file without a loaded category.
  auto const written = RunAndWait<bool>(Platform::Thread::File, [&]
  {
    bool matches = false;
    bool const unchanged = update->PerformIfUnchanged([&]
    {
      matches = Matches(path, expectedHash);
      if (!matches)
        return;
      if (!Platform::MkDirChecked(m_dir))
        MYTHROW(RootException, ("Can't create", m_dir));
      // In the same step, so an interrupted pass never leaves the replacement without the copy.
      if (device)
        WriteConflictCopy(path, copyPath, *device);
      if (!CopyAtomically(incoming->GetPath(), path))
        MYTHROW(RootException, ("Can't replace", path));
    });
    return unchanged && matches;
  });
  if (!written)
    return ApplyResult::Deferred;
  if (!*written)
    return ApplyResult::Changed;

  struct Finished
  {
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::optional<bool> m_applied;
    bool m_admitted = false;
  };
  auto finished = std::make_shared<Finished>();
  auto & manager = m_manager;
  GetPlatform().RunTask(Platform::Thread::Gui, [&manager, path, update, incoming, copyPath, canApply, finished]
  {
    // |incoming| stays until core has parsed it, also after the pass stopped waiting.
    manager.ReloadBookmark(update, incoming->GetPath(),
                           [&manager, path, incoming, copyPath, canApply, finished](bool applied)
    {
      // A new file has no category that core could save over it: load it anyway, it holds no ids yet.
      if (!applied && manager.GetCategoryByFileName(path) == kml::kInvalidMarkGroupId)
        manager.LoadBookmarkIfNotLoaded(path);
      if (!copyPath.empty())
      {
        if (applied)
          manager.LoadBookmarkIfNotLoaded(copyPath);
        else  // After the save that core has queued to restore the file, so a crash never loses the local version.
          GetPlatform().RunTask(Platform::Thread::File, [copyPath] { base::DeleteFileX(copyPath); });
      }
      bool const admitted = !applied && !manager.IsAsyncLoadingInProgress() && manager.AreNotificationsEnabled() &&
                            (!canApply || canApply());
      {
        std::lock_guard lock(finished->m_mutex);
        finished->m_applied = applied;
        finished->m_admitted = admitted;
      }
      finished->m_cv.notify_all();
    });
  });

  std::unique_lock lock(finished->m_mutex);
  // Core applies or rejects the file later, and the next pass sees either result.
  if (!finished->m_cv.wait_for(lock, kStartTimeout, [&finished] { return finished->m_applied.has_value(); }))
    return ApplyResult::Changed;
  if (*finished->m_applied)
    return ApplyResult::Applied;
  // Core rejected the update: a local edit wins until the next pass, or a screen opened meanwhile.
  return finished->m_admitted ? ApplyResult::Changed : ApplyResult::Deferred;
}

ApplyResult LocalStore::InstallUnique(std::string const & name, StagedFilePtr const & incoming,
                                      std::set<std::string> const & reserved, CanApply const & canApply) const
{
  if (!LoadKmlFile(incoming->GetPath(), FileType::Kml))
  {
    LOG(LWARNING, ("Can't parse a duplicate of", name));
    return ApplyResult::Ignored;
  }
  auto taken = reserved;
  taken.insert(name);
  auto const path = RunAndWait<std::string>(Platform::Thread::Gui, [&]
  {
    if (!IsAdmitted(canApply))
      return std::string{};
    return ChooseFreePath(base::GetNameFromFullPathWithoutExt(name), taken);
  });
  if (!path || path->empty())
    return ApplyResult::Deferred;

  auto const written = RunAndWait<bool>(Platform::Thread::File, [&]
  {
    // Taken by an import queued meanwhile.
    if (Platform::IsFileExistsByFullPath(*path))
      return false;
    if (!Platform::MkDirChecked(m_dir) || !CopyAtomically(incoming->GetPath(), *path))
      MYTHROW(RootException, ("Can't write", *path));
    return true;
  });
  if (!written)
    return ApplyResult::Deferred;
  if (!*written)
    return ApplyResult::Changed;
  GetPlatform().RunTask(Platform::Thread::Gui,
                        [&manager = m_manager, path = *path] { manager.LoadBookmarkIfNotLoaded(path); });
  return ApplyResult::Applied;
}

ApplyResult LocalStore::Trash(std::string const & name, std::string const & expectedHash,
                              CanApply const & canApply) const
{
  auto const path = GetPath(name);
  struct Prepared
  {
    kml::MarkGroupId m_groupId = kml::kInvalidMarkGroupId;
    BookmarkManager::FileUpdatePtr m_update;
    bool m_admitted = false;
  };
  auto const prepared = RunAndWait<Prepared>(Platform::Thread::Gui, [&]
  {
    Prepared result;
    result.m_admitted = IsAdmitted(canApply);
    if (result.m_admitted)
    {
      result.m_groupId = m_manager.GetCategoryByFileName(path);
      result.m_update = m_manager.GetFileUpdate(path);
    }
    return result;
  });
  if (!prepared || !prepared->m_admitted)
    return ApplyResult::Deferred;
  if (!prepared->m_update)
    return ApplyResult::Changed;

  // Hashed here, not on the File thread: an autosave of this category waits meanwhile, other core work doesn't.
  bool matches = false;
  if (!prepared->m_update->PerformIfUnchanged([&] { matches = Matches(path, expectedHash); }) || !matches)
    return ApplyResult::Changed;

  auto const result = RunAndWait<ApplyResult>(Platform::Thread::Gui, [&]
  {
    if (!IsAdmitted(canApply))
      return ApplyResult::Deferred;
    // Still the same category, and no save requested since the hash check.
    if (m_manager.GetCategoryByFileName(path) != prepared->m_groupId || !prepared->m_update->PerformIfUnchanged([] {}))
      return ApplyResult::Changed;
    if (prepared->m_groupId == kml::kInvalidMarkGroupId)
    {
      // An unloaded file, like an unreadable one: there is no category to delete, only its file.
      auto const trashedPath = GenerateValidAndUniqueTrashedFilePath(name);
      if (!base::MoveFileX(path, trashedPath))
        MYTHROW(RootException, ("Can't move", path, "to", trashedPath));
      return ApplyResult::Applied;
    }
    if (!m_manager.GetEditSession().DeleteBmCategory(prepared->m_groupId, false /* permanently */))
    {
      LOG(LWARNING, ("Can't trash", path));
      return ApplyResult::Changed;
    }
    return ApplyResult::Applied;
  });
  return result.value_or(ApplyResult::Deferred);
}

std::string LocalStore::GetPath(std::string const & name) const
{
  return base::JoinPath(m_dir, name);
}

bool LocalStore::IsAdmitted(CanApply const & canApply) const
{
  return !m_manager.IsAsyncLoadingInProgress() && m_manager.AreNotificationsEnabled() && (!canApply || canApply());
}

bool LocalStore::Matches(std::string const & path, std::optional<std::string> const & expectedHash) const
{
  if (!expectedHash)
    return !Platform::IsFileExistsByFullPath(path);
  try
  {
    return HashFile(path, m_algorithm) == *expectedHash;
  }
  catch (Reader::Exception const &)
  {
    return false;
  }
}

std::string LocalStore::ChooseFreePath(std::string const & stem, std::set<std::string> const & reserved) const
{
  auto const validStem = TruncateToValidFileName(RemoveInvalidSymbols(stem));
  auto const isFree = [&](std::string const & fileName)
  {
    auto const path = GetPath(fileName);
    // A loaded category may not have reached the disk yet.
    return !reserved.contains(fileName) && !Platform::IsFileExistsByFullPath(path) &&
           m_manager.GetCategoryByFileName(path) == kml::kInvalidMarkGroupId;
  };
  auto fileName = validStem + std::string{kKmlExtension};
  for (size_t i = 1; !isFree(fileName); ++i)
    fileName = validStem + std::to_string(i) + std::string{kKmlExtension};
  return GetPath(fileName);
}

bool IsUntouchedList(std::string const & path)
{
  auto const data = LoadKmlFile(path, FileType::Kml);
  return data && data->m_bookmarksData.empty() && data->m_tracksData.empty() &&
         data->m_categoryData.m_description.empty() && data->m_categoryData.m_annotation.empty();
}
}  // namespace bookmarks_sync
