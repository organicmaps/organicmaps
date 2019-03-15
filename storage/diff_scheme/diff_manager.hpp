#pragma once

#include "storage/diff_scheme/diff_types.hpp"
#include "storage/storage_defines.hpp"

#include "generator/mwm_diff/diff.hpp"

#include "base/observer_list.hpp"
#include "base/thread_checker.hpp"
#include "base/thread_pool_delayed.hpp"

#include <functional>
#include <mutex>
#include <utility>

namespace base
{
class Cancellable;
}

namespace storage
{
namespace diffs
{
class Manager final
{
public:
  struct ApplyDiffParams
  {
    string m_diffReadyPath;
    LocalFilePtr m_diffFile;
    LocalFilePtr m_oldMwmFile;
  };

  class Observer
  {
  public:
    virtual ~Observer() = default;

    virtual void OnDiffStatusReceived(Status const status) = 0;
  };

  using OnDiffApplicationFinished = std::function<void(generator::mwm_diff::DiffApplicationResult)>;

  // If the diff is available, sets |size| to its size and returns true.
  // Otherwise, returns false.
  bool SizeFor(storage::CountryId const & countryId, uint64_t & size) const;

  // Sets |size| to how many bytes are left for the diff to be downloaded for |countryId|
  // or 0 if there is no diff available or it has already been downloaded.
  // This method may overestimate because it does not account for the possibility
  // of resuming an old download, i.e. the return value is either 0 or the diff size.
  // Returns true iff the diff is available.
  bool SizeToDownloadFor(storage::CountryId const & countryId, uint64_t & size) const;

  bool VersionFor(storage::CountryId const & countryId, uint64_t & version) const;
  bool IsPossibleToAutoupdate() const;

  // Checks whether the diff for |countryId| is available for download or
  // has been downloaded.
  bool HasDiffFor(storage::CountryId const & countryId) const;

  void RemoveDiffForCountry(storage::CountryId const & countryId);
  void AbortDiffScheme();

  Status GetStatus() const;

  void Load(LocalMapsInfo && info);
  void ApplyDiff(ApplyDiffParams && p, base::Cancellable const & cancellable,
                 OnDiffApplicationFinished const & task);

  bool AddObserver(Observer & observer) { return m_observers.Add(observer); }
  bool RemoveObserver(Observer const & observer) { return m_observers.Remove(observer); }

private:
  template <typename Fn>
  bool WithDiff(storage::CountryId const & countryId, Fn && fn) const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_status != Status::Available)
      return false;

    auto const it = m_diffs.find(countryId);
    if (it == m_diffs.cend())
      return false;

    fn(it->second);
    return true;
  }

  mutable std::mutex m_mutex;
  Status m_status = Status::Undefined;
  NameDiffInfoMap m_diffs;
  LocalMapsInfo m_localMapsInfo;
  base::ObserverListUnsafe<Observer> m_observers;
  base::thread_pool::delayed::ThreadPool m_workerThread;
};
}  // namespace diffs
}  // namespace storage
