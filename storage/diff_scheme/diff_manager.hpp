#pragma once

#include "storage/diff_scheme/diff_types.hpp"
#include "storage/index.hpp"

#include "base/observer_list.hpp"
#include "base/thread_checker.hpp"
#include "base/worker_thread.hpp"

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
    TLocalFilePtr m_diffFile;
    TLocalFilePtr m_oldMwmFile;
  };

  class Observer
  {
  public:
    virtual ~Observer() = default;

    virtual void OnDiffStatusReceived() = 0;
  };

  FileInfo const & InfoFor(storage::TCountryId const & countryId) const;
  bool IsPossibleToAutoupdate() const;
  bool HasDiffFor(storage::TCountryId const & countryId) const;

  Status GetStatus() const;
  void SetStatus(Status status);

  void Load(LocalMapsInfo && info);
  void ApplyDiff(ApplyDiffParams && p, std::function<void(bool const result)> const & task);

  bool AddObserver(Observer & observer) { return m_observers.Add(observer); }
  bool RemoveObserver(Observer const & observer) { return m_observers.Remove(observer); }

private:
  bool HasDiffForUnsafe(storage::TCountryId const & countryId) const;

  mutable std::mutex m_mutex;
  Status m_status = Status::Undefined;
  NameFileInfoMap m_diffs;
  LocalMapsInfo m_localMapsInfo;
  base::ObserverListUnsafe<Observer> m_observers;
  base::WorkerThread m_workerThread;
};
}  // namespace diffs
}  // namespace storage
