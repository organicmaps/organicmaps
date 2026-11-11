#include "map/bookmarks_sync/sync_engine.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/bookmarks_sync/base_state.hpp"
#include "map/bookmarks_sync/content_hash.hpp"
#include "map/bookmarks_sync/reconciler.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include <chrono>
#include <map>
#include <set>
#include <unordered_set>

namespace bookmarks_sync
{
namespace
{
int64_t constexpr kPurgeIntervalMs = 24LL * 60 * 60 * 1000;
int64_t constexpr kTrashRetentionMs = 30LL * 24 * 60 * 60 * 1000;

int64_t NowMs()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

// Listing the trash costs as much quota as a whole idle pass, and a day changes nothing for a 30-day retention.
// A clock moved back purges too, so a time in the future can't stop it.
bool IsPurgeDue(std::optional<int64_t> lastPurgeMs, int64_t nowMs)
{
  return !lastPurgeMs || nowMs < *lastPurgeMs || nowMs - *lastPurgeMs >= kPurgeIntervalMs;
}

enum class Outcome
{
  Done,
  Retry,
  Deferred,
  // The cloud data is being deleted: nothing else is applied until the next pass sees the new generation.
  Abort,
};

Outcome ToOutcome(ApplyResult result)
{
  switch (result)
  {
  case ApplyResult::Applied:
  case ApplyResult::Ignored: return Outcome::Done;
  case ApplyResult::Changed: return Outcome::Retry;
  case ApplyResult::Deferred: return Outcome::Deferred;
  }
  UNREACHABLE();
}
}  // namespace

class Pass
{
public:
  Pass(SyncEngine & engine, CloudStore & cloud, LocalStore const & local, CanApply const & canApply)
    : m_engine(engine)
    , m_cloud(cloud)
    , m_local(local)
    , m_canApply(canApply)
  {}

  PassResult::Status Run()
  {
    auto listed = m_cloud.List();
    CheckCancelled();
    std::optional<CloudFile> generation;
    for (auto const & file : listed)
      if (file.m_name == kGenerationFileName && (!generation || IsOlder(file, *generation)))
        generation = file;

    m_state = m_saved = LoadBaseState(m_engine.m_statePath, m_engine.m_algorithm);
    std::vector<CloudFile> files;
    std::string generationId;
    if (generation)
    {
      generationId = generation->m_id;
      // Older files are left from deleted data, which another device may still be deleting one by one: they are
      // uploaded again rather than matched.
      for (auto & file : listed)
        if (file.m_name.ends_with(kKmlExtension) && file.m_createdTimeMs >= generation->m_createdTimeMs)
          files.push_back(std::move(file));
    }
    else
    {
      // A listing that lags behind can miss the generation of the base.
      if (m_state.m_generation && m_cloud.Stat(*m_state.m_generation))
        return PassResult::Status::Retry;
      generationId = m_cloud.CreateGeneration();
    }
    // Another account, or all data deleted from another device or in the cloud settings: the cloud files share no
    // history with the base, so a missing one must not turn into a local deletion.
    if (m_state.m_generation != generationId)
    {
      m_state = {.m_algorithm = m_engine.m_algorithm, .m_generation = generationId};
      Save();
    }

    FillHashes(files);
    RecoverUploads(files);
    auto const local = m_local.Snapshot(!m_state.m_files.empty());
    for (auto const & file : files)
      m_cloudNames.insert(file.m_name);

    bool deferred = false;
    bool retry = false;
    for (auto const & action : Plan(local, files, m_state.m_files))
    {
      CheckCancelled();
      auto const outcome = std::visit([this](auto const & a) { return Apply(a); }, action);
      deferred = deferred || outcome == Outcome::Deferred;
      retry = retry || outcome == Outcome::Retry || outcome == Outcome::Abort;
      if (outcome == Outcome::Abort)
        break;
    }

    CheckCancelled();
    auto const now = NowMs();
    if (IsPurgeDue(m_state.m_lastPurgeMs, now))
    {
      m_cloud.PurgeTrash(now - kTrashRetentionMs);
      m_state.m_lastPurgeMs = now;
      Save();
    }
    if (deferred)
      return PassResult::Status::Deferred;
    if (retry)
      return PassResult::Status::Retry;
    // Shown as the last sync, so it marks only passes that left nothing for later.
    m_state.m_lastSyncMs = now;
    Save();
    return PassResult::Status::Synced;
  }

private:
  void CheckCancelled() const
  {
    if (m_engine.m_cancelled)
      MYTHROW1(CloudException, SyncError::Cancelled, ("The pass is cancelled"));
  }

  void Save()
  {
    if (m_state == m_saved)
      return;
    SaveBaseState(m_engine.m_statePath, m_state);
    m_saved = m_state;
    m_engine.m_lastSyncMs = m_state.m_lastSyncMs.value_or(0);
  }

  BaseEntry const * FindBase(std::string const & name) const
  {
    auto const it = m_state.m_files.find(name);
    return it == m_state.m_files.end() ? nullptr : &it->second;
  }

  void WithFile(std::string const & name, CloudFile const & file, std::string const & hash)
  {
    m_state.m_files[name] = {file.m_id, hash, file.m_version};
    m_state.m_pendingUploads.erase(name);
    Save();
  }

  void WithoutFile(std::string const & name)
  {
    m_state.m_files.erase(name);
    m_state.m_pendingUploads.erase(name);
    Save();
  }

  /// Without hashes from the store, a version is known only from the base, and other files are hashed once they
  /// are downloaded, which most of them would need anyway.
  void FillHashes(std::vector<CloudFile> & files)
  {
    std::erase_if(files, [this](CloudFile & file)
    {
      if (!m_engine.m_cloudAlgorithm)
        file.m_hash.reset();
      if (file.m_hash)
        return false;
      if (auto const * base = FindBase(file.m_name);
          base && base->m_cloudId == file.m_id && base->m_version == file.m_version)
      {
        file.m_hash = base->m_hash;
        return false;
      }
      CheckCancelled();
      auto staged = m_local.NewStagedFile();
      // Deleted after it was listed: the next pass sees the deletion.
      if (!m_cloud.Download(file.m_id, staged->GetPath()))
        return true;
      file.m_hash = HashFile(staged->GetPath(), m_engine.m_algorithm);
      m_downloaded.emplace(file.m_id, std::move(staged));
      return false;
    });
  }

  /// An upload can reach the cloud while its result is lost: the pass was cancelled, the process killed or the
  /// connection dropped. Recognized by its hash, it is recorded as synced, not taken for another device's version.
  void RecoverUploads(std::vector<CloudFile> const & files)
  {
    auto const pending = m_state.m_pendingUploads;
    for (auto const & [name, hash] : pending)
    {
      CloudFile const * found = nullptr;
      for (auto const & file : files)
        if (file.m_name == name && file.m_hash == hash && (!found || IsOlder(file, *found)))
          found = &file;
      if (found)
        WithFile(name, *found, hash);
    }
  }

  /// Nothing if the file changed or was deleted after it was listed; the next pass sees the new state.
  StagedFilePtr Fetch(CloudFile const & file)
  {
    if (auto const it = m_downloaded.find(file.m_id); it != m_downloaded.end())
      return it->second;
    auto staged = m_local.NewStagedFile();
    if (!m_cloud.Download(file.m_id, staged->GetPath()))
      return nullptr;
    if (m_engine.m_cloudAlgorithm && HashFile(staged->GetPath(), m_engine.m_algorithm) != file.m_hash)
      return nullptr;
    return staged;
  }

  bool CanApplyChanges() const { return m_local.CanApplyChanges(m_canApply); }

  /// Saves the hash first, so that the next pass recognizes the uploaded file if the result is lost.
  Outcome UploadFile(std::string const & name, std::optional<std::string> const & existingId)
  {
    // One immutable copy is hashed and uploaded, even if core replaces the file meanwhile.
    auto const staged = m_local.Stage(name);
    // Deleted since the snapshot: the next pass sees the deletion.
    if (!staged)
      return Outcome::Retry;
    auto const hash = HashFile(staged->GetPath(), m_engine.m_algorithm);
    m_state.m_pendingUploads[name] = hash;
    Save();
    CheckCancelled();
    auto const uploaded = m_cloud.Upload(name, staged->GetPath(), existingId);
    if (m_engine.m_cloudAlgorithm && uploaded.m_hash != hash)
      LOG(LWARNING, ("The cloud reports another hash for", name, uploaded, hash));
    WithFile(name, uploaded, hash);
    return Outcome::Done;
  }

  Outcome Install(std::string const & name, StagedFilePtr const & staged,
                  std::optional<std::string> const & expectedHash, CloudFile const & file)
  {
    auto const result = m_local.Install(name, staged, expectedHash, m_canApply);
    if (result == ApplyResult::Applied)
      WithFile(name, file, *file.m_hash);
    return ToOutcome(result);
  }

  Outcome Apply(action::Upload const & a)
  {
    // Looked up by id, as a listing can lag behind and miss a file that still exists.
    std::optional<CloudFile> current;
    if (a.m_cloudId)
      current = m_cloud.Stat(*a.m_cloudId);
    // Overwrite only the version unchanged since the last sync; a file that is gone is created again.
    auto const * base = FindBase(a.m_name);
    if (current && (!base || current->m_version != base->m_version))
      return Outcome::Retry;
    return UploadFile(a.m_name, current ? a.m_cloudId : std::nullopt);
  }

  Outcome Apply(action::Download const & a)
  {
    // A listing that lags behind must not replace a list: the synced cloud file has to be really gone.
    if (a.m_replacedCloudId && m_cloud.Stat(*a.m_replacedCloudId))
      return Outcome::Retry;
    if (!CanApplyChanges())
      return Outcome::Deferred;
    auto const staged = Fetch(a.m_cloud);
    if (!staged)
      return Outcome::Retry;
    return Install(a.m_name, staged, a.m_expectedLocalHash, a.m_cloud);
  }

  Outcome Apply(action::ConflictCopy const & a)
  {
    if (!CanApplyChanges())
      return Outcome::Deferred;
    auto const staged = Fetch(a.m_cloud);
    if (!staged)
      return Outcome::Retry;
    if (!a.m_hasBase)
    {
      // Never synchronized: an untouched list, like the default one of a new device, is not worth a copy.
      if (IsUntouchedList(m_local.GetPath(a.m_name)))
        return Install(a.m_name, staged, a.m_localHash, a.m_cloud);
      if (IsUntouchedList(staged->GetPath()))
      {
        auto const current = m_cloud.Stat(a.m_cloud.m_id);
        if (!current || current->m_version != a.m_cloud.m_version)
          return Outcome::Retry;
        return UploadFile(a.m_name, a.m_cloud.m_id);
      }
    }
    auto const result =
        m_local.ResolveConflict(a.m_name, staged, a.m_localHash, m_engine.m_deviceName, m_cloudNames, m_canApply);
    if (result == ApplyResult::Applied)
      WithFile(a.m_name, a.m_cloud, *a.m_cloud.m_hash);
    return ToOutcome(result);
  }

  Outcome Apply(action::TrashCloud const & a)
  {
    auto const current = m_cloud.Stat(a.m_cloudId);
    if (current)
    {
      // Edited elsewhere: the next pass downloads it.
      if (current->m_version != a.m_expectedVersion)
        return Outcome::Retry;
      m_cloud.MoveToTrash(a.m_cloudId);
    }
    WithoutFile(a.m_name);
    return Outcome::Done;
  }

  Outcome Apply(action::TrashLocal const & a)
  {
    // A listing that lags behind must not delete a list: trash it only once the cloud file is really gone, but not
    // with its generation, as during a deletion of all data.
    if (m_cloud.Stat(a.m_cloudId))
      return Outcome::Retry;
    if (!m_cloud.Stat(*m_state.m_generation))
      return Outcome::Abort;
    auto const result = m_local.Trash(a.m_name, a.m_expectedHash, m_canApply);
    if (result == ApplyResult::Applied)
      WithoutFile(a.m_name);
    return ToOutcome(result);
  }

  Outcome Apply(action::RecordBase const & a)
  {
    m_state.m_files[a.m_name] = a.m_entry;
    Save();
    return Outcome::Done;
  }

  Outcome Apply(action::ResolveDuplicate const & a)
  {
    // The base still refers to it: trashed only once the action for the name has kept or replaced this version.
    if (auto const * base = FindBase(a.m_name); base && base->m_cloudId == a.m_duplicate.m_id)
      return Outcome::Done;
    if (a.m_keepAsLocalCopy)
    {
      if (!CanApplyChanges())
        return Outcome::Deferred;
      auto const staged = Fetch(a.m_duplicate);
      if (!staged)
        return Outcome::Retry;
      if (!IsUntouchedList(staged->GetPath()))
      {
        auto const result = m_local.InstallUnique(a.m_name, staged, m_cloudNames, m_canApply);
        if (result != ApplyResult::Applied)
          return ToOutcome(result);
      }
    }
    m_cloud.MoveToTrash(a.m_duplicate.m_id);
    return Outcome::Done;
  }

  SyncEngine & m_engine;
  CloudStore & m_cloud;
  LocalStore const & m_local;
  CanApply const & m_canApply;
  BaseState m_state;
  BaseState m_saved;
  std::set<std::string> m_cloudNames;
  // Downloaded to learn their hashes, kept for the actions.
  std::map<std::string, StagedFilePtr> m_downloaded;
};

std::string DebugPrint(PassResult::Status status)
{
  switch (status)
  {
  case PassResult::Status::Synced: return "Synced";
  case PassResult::Status::Deferred: return "Deferred";
  case PassResult::Status::Retry: return "Retry";
  case PassResult::Status::Failed: return "Failed";
  }
  UNREACHABLE();
}

std::string DebugPrint(PassResult const & result)
{
  if (result.m_status != PassResult::Status::Failed)
    return DebugPrint(result.m_status);
  return "Failed [" + DebugPrint(result.m_error) + ", " + result.m_message + "]";
}

SyncEngine::SyncEngine(BookmarkManager & manager, std::string bookmarksDir, std::string statePath,
                       std::string stagingDir, std::string deviceName, std::optional<HashAlgorithm> cloudAlgorithm)
  : m_manager(manager)
  , m_bookmarksDir(std::move(bookmarksDir))
  , m_statePath(std::move(statePath))
  , m_stagingDir(std::move(stagingDir))
  , m_deviceName(std::move(deviceName))
  , m_cloudAlgorithm(cloudAlgorithm)
  , m_algorithm(cloudAlgorithm.value_or(HashAlgorithm::Blake3))
{
  // Left by a process that was killed during a pass.
  LocalStore::CleanStaging(m_stagingDir);
  try
  {
    m_lastSyncMs = LoadBaseState(m_statePath, m_algorithm).m_lastSyncMs.value_or(0);
  }
  catch (RootException const & e)
  {
    LOG(LWARNING, ("Can't read the sync state", e.Msg()));
  }
}

PassResult SyncEngine::RunPass(CloudStore & cloud, CanApply const & canApply)
{
  CHECK(cloud.GetHashAlgorithm() == m_cloudAlgorithm, (m_statePath));
  std::lock_guard lock(m_mutex);
  m_cancelled = false;
  m_running = true;
  SCOPE_GUARD(stopRunning, [this] { m_running = false; });

  LocalStore const local(m_manager, m_bookmarksDir, m_stagingDir, m_algorithm);
  PassResult result;
  try
  {
    result.m_status = Pass(*this, cloud, local, canApply).Run();
  }
  catch (CloudException const & e)
  {
    result = {PassResult::Status::Failed, e.GetError(), e.Msg()};
  }
  catch (RootException const & e)
  {
    result = {PassResult::Status::Failed, SyncError::Other, e.Msg()};
  }
  // A request aborted by the cancellation fails like a network error.
  if (result.m_status == PassResult::Status::Failed && m_cancelled)
    result.m_error = SyncError::Cancelled;
  LOG(LINFO, ("Bookmarks sync pass:", result));
  return result;
}

void SyncEngine::DeleteCloudData(CloudStore & cloud)
{
  std::lock_guard lock(m_mutex);
  cloud.DeleteAll(LoadBaseState(m_statePath, m_algorithm).m_generation);
  DeleteBaseState(m_statePath);
  m_lastSyncMs = 0;
}

void SyncEngine::ForgetMissingLocalFiles()
{
  std::lock_guard lock(m_mutex);
  LocalStore const local(m_manager, m_bookmarksDir, m_stagingDir, m_algorithm);
  auto const namesList = local.ListNames();
  std::unordered_set<std::string> const names(namesList.begin(), namesList.end());
  auto state = LoadBaseState(m_statePath, m_algorithm);
  auto const before = state;
  std::erase_if(state.m_files, [&names](auto const & entry) { return !names.contains(entry.first); });
  std::erase_if(state.m_pendingUploads, [&names](auto const & entry) { return !names.contains(entry.first); });
  if (state != before)
    SaveBaseState(m_statePath, state);
}

std::optional<int64_t> SyncEngine::GetLastSyncMs() const
{
  int64_t const lastSync = m_lastSyncMs;
  return lastSync == 0 ? std::nullopt : std::optional(lastSync);
}
}  // namespace bookmarks_sync
