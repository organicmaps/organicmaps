#pragma once

#include "map/bookmarks_sync/cloud_store.hpp"
#include "map/bookmarks_sync/reconciler.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

class BookmarkManager;

namespace bookmarks_sync
{
/// Called on the GUI thread: false while replacing a category would break a screen that keeps its ids.
using CanApply = std::function<bool()>;

/// A file outside the bookmarks directory, deleted with its last reference. Immutable once written, so that core
/// can parse it while the original changes.
class StagedFile
{
public:
  explicit StagedFile(std::string path) : m_path(std::move(path)) {}
  ~StagedFile();
  StagedFile(StagedFile const &) = delete;
  StagedFile & operator=(StagedFile const &) = delete;

  std::string const & GetPath() const { return m_path; }

private:
  std::string m_path;
};
using StagedFilePtr = std::shared_ptr<StagedFile>;

enum class ApplyResult
{
  Applied,
  // Nothing to retry, like an unreadable incoming file.
  Ignored,
  // The local file or its category changed since the plan.
  Changed,
  // A screen keeps the category, its edits aren't saved yet, or core is busy loading.
  Deferred,
};

std::string DebugPrint(ApplyResult result);

/// The flat directory of category files that core loads. Every change re-checks the hash it was planned with and
/// goes through core, so a queued save, a local edit or an open screen is never overwritten.
class LocalStore
{
public:
  LocalStore(BookmarkManager & manager, std::string dir, std::string stagingDir, HashAlgorithm algorithm);

  static void CleanStaging(std::string const & stagingDir);

  /// Waits for the saves queued so far, so a pass started right after a change sees it. Throws if the directory is
  /// missing while |requireDir|.
  std::vector<LocalFile> Snapshot(bool requireDir) const;
  std::vector<std::string> ListNames() const;

  bool CanApplyChanges(CanApply const & canApply) const;

  StagedFilePtr NewStagedFile() const;
  /// A copy of the file taken after the queued saves, or nothing if it is gone.
  StagedFilePtr Stage(std::string const & name) const;

  /// Replaces the file with |incoming| if its hash is still |expectedHash| (nothing: it must not exist).
  ApplyResult Install(std::string const & name, StagedFilePtr const & incoming,
                      std::optional<std::string> const & expectedHash, CanApply const & canApply) const;
  /// Keeps the local version as "Name (device).kml" with its category named "Name (device)", then installs
  /// |incoming|. |reserved| names are taken in the cloud.
  ApplyResult ResolveConflict(std::string const & name, StagedFilePtr const & incoming,
                              std::string const & expectedHash, std::string const & device,
                              std::set<std::string> const & reserved, CanApply const & canApply) const;
  /// Loads |incoming| as a new category under a free name derived from |name|, never |name| itself.
  ApplyResult InstallUnique(std::string const & name, StagedFilePtr const & incoming,
                            std::set<std::string> const & reserved, CanApply const & canApply) const;
  ApplyResult Trash(std::string const & name, std::string const & expectedHash, CanApply const & canApply) const;

  std::string GetPath(std::string const & name) const;

private:
  ApplyResult Replace(std::string const & name, StagedFilePtr const & incoming,
                      std::optional<std::string> const & expectedHash, std::string const * device,
                      std::set<std::string> const & reserved, CanApply const & canApply) const;
  bool IsAdmitted(CanApply const & canApply) const;
  bool Matches(std::string const & path, std::optional<std::string> const & expectedHash) const;
  std::string ChooseFreePath(std::string const & stem, std::set<std::string> const & reserved) const;

  BookmarkManager & m_manager;
  std::string m_dir;
  std::string m_stagingDir;
  HashAlgorithm m_algorithm;
};

/// True for a category without bookmarks, tracks, description and annotation, like an untouched default list.
bool IsUntouchedList(std::string const & path);
}  // namespace bookmarks_sync
