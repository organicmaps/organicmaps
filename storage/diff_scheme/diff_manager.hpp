#pragma once

#include "storage/diff_scheme/diff_types.hpp"
#include "storage/storage_defines.hpp"

#include "base/observer_list.hpp"
#include "base/thread_checker.hpp"
#include "base/thread_pool_delayed.hpp"

#include <functional>
#include <mutex>
#include <utility>

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

  bool SizeFor(storage::CountryId const & countryId, uint64_t & size) const;
  bool VersionFor(storage::CountryId const & countryId, uint64_t & version) const;
  bool IsPossibleToAutoupdate() const;
  bool HasDiffFor(storage::CountryId const & countryId) const;
  void RemoveAppliedDiffs();
  void AbortDiffScheme();

  Status GetStatus() const;

  void Load(LocalMapsInfo && info);
  void ApplyDiff(ApplyDiffParams && p, std::function<void(bool const result)> const & task);

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
