#include "storage/diff_scheme/diff_manager.hpp"
#include "storage/diff_scheme/diff_scheme_checker.hpp"

#include "generator/mwm_diff/diff.hpp"

#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

#include "base/assert.hpp"

namespace storage
{
namespace diffs
{
void Manager::Load(LocalMapsInfo && info)
{
  LocalMapsInfo localMapsInfo = info;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_localMapsInfo = std::move(info);
  }

  m_workerThread.Push([this, localMapsInfo]
  {
    NameDiffInfoMap const diffs = Checker::Check(localMapsInfo);

    std::lock_guard<std::mutex> lock(m_mutex);

    m_diffs = diffs;
    if (diffs.empty())
    {
      m_status = Status::NotAvailable;

      GetPlatform().GetMarketingService().SendMarketingEvent(marketing::kDiffSchemeFallback, {});
    }
    else
    {
      m_status = Status::Available;
    }

    auto & observers = m_observers;
    auto status = m_status;
    GetPlatform().RunTask(Platform::Thread::Gui, [observers, status]() mutable {
      observers.ForEach(&Observer::OnDiffStatusReceived, status);
    });
  });
}

void Manager::ApplyDiff(ApplyDiffParams && p, std::function<void(bool const result)> const & task)
{
  m_workerThread.Push([this, p, task]
  {
    CHECK(p.m_diffFile, ());
    CHECK(p.m_oldMwmFile, ());

    auto & diffReadyPath = p.m_diffReadyPath;
    auto & diffFile = p.m_diffFile;
    auto const diffPath = diffFile->GetPath(MapOptions::Diff);
    bool result = false;

    diffFile->SyncWithDisk();

    auto const isOnDisk = diffFile->OnDisk(MapOptions::Diff);
    auto const isFilePrepared = isOnDisk || my::RenameFileX(diffReadyPath, diffPath);

    if (isFilePrepared)
    {
      // Sync with disk after renaming.
      if (!isOnDisk)
        diffFile->SyncWithDisk();

      string const oldMwmPath = p.m_oldMwmFile->GetPath(MapOptions::Map);
      string const newMwmPath = diffFile->GetPath(MapOptions::Map);
      string const diffApplyingInProgressPath = newMwmPath + DIFF_APPLYING_FILE_EXTENSION;
      result = generator::mwm_diff::ApplyDiff(oldMwmPath, diffApplyingInProgressPath, diffPath) &&
              my::RenameFileX(diffApplyingInProgressPath, newMwmPath);
    }

    diffFile->DeleteFromDisk(MapOptions::Diff);

    if (result)
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      auto it = m_diffs.find(diffFile->GetCountryName());
      CHECK(it != m_diffs.end(), ());
      it->second.m_applied = true;
    }
    else
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_status = Status::NotAvailable;

      GetPlatform().GetMarketingService().SendMarketingEvent(
          marketing::kDiffSchemeError,
          {{"type", "patching"},
           {"error", isFilePrepared ? "Cannot apply diff" : "Cannot prepare file"}});
    }

    task(result);
  });
}

Status Manager::GetStatus() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_status;
}

bool Manager::SizeFor(storage::TCountryId const & countryId, uint64_t & size) const
{
  return WithDiff(countryId, [&size](DiffInfo const & info) { size = info.m_size; });
}

bool Manager::VersionFor(storage::TCountryId const & countryId, uint64_t & version) const
{
  return WithDiff(countryId, [&version](DiffInfo const & info) { version = info.m_version; });
}

bool Manager::HasDiffFor(storage::TCountryId const & countryId) const
{
  return WithDiff(countryId, [](DiffInfo const &){});
}

void Manager::RemoveAppliedDiffs()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto it = m_diffs.begin(); it != m_diffs.end();)
  {
    if (it->second.m_applied)
      it = m_diffs.erase(it);
    else
      ++it;
  }

  if (m_diffs.empty())
    m_status = Status::NotAvailable;
}

void Manager::AbortDiffScheme()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_status = Status::NotAvailable;
  m_diffs.clear();
}

bool Manager::IsPossibleToAutoupdate() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (m_status != Status::Available)
    return false;

  for (auto const & nameVersion : m_localMapsInfo.m_localMaps)
  {
    if (m_diffs.find(nameVersion.first) == m_diffs.end())
      return false;
  }
  return true;
}
}  // namespace diffs
}  // namespace storage
