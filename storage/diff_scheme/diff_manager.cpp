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
    NameFileInfoMap const diffs = Checker::Check(localMapsInfo);

    std::lock_guard<std::mutex> lock(m_mutex);

    m_diffs = diffs;
    if (diffs.empty())
    {
      m_status = Status::NotAvailable;
      // TODO: Log fall back to the old scheme (Downloader_DiffScheme_OnStart_fallback (Aloha)).
    }
    else
    {
      m_status = Status::Available;
    }

    auto & observers = m_observers;
    GetPlatform().RunOnGuiThread([observers]() mutable
    {
      observers.ForEach(&Observer::OnDiffStatusReceived);
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
    bool result = false;

    if (!my::RenameFileX(diffReadyPath, diffFile->GetPath(MapOptions::Diff)))
    {
      diffFile->SyncWithDisk();
      diffFile->DeleteFromDisk(MapOptions::Diff);
    }
    else
    {
      diffFile->SyncWithDisk();

      string const oldMwmPath = p.m_oldMwmFile->GetPath(MapOptions::Map);
      string const newMwmPath = diffFile->GetPath(MapOptions::Map);
      string const diffPath = diffFile->GetPath(MapOptions::Diff);
      result = generator::mwm_diff::ApplyDiff(oldMwmPath, newMwmPath, diffPath);
      diffFile->DeleteFromDisk(MapOptions::Diff);
    }

    if (!result)
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_status = Status::NotAvailable;
      // TODO: Log the diff applying error (Downloader_DiffScheme_error (Aloha)).
    }

    task(result);
  });
}

Status Manager::GetStatus() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_status;
}

void Manager::SetStatus(Status status)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_status = status;
}

FileInfo const & Manager::InfoFor(storage::TCountryId const & countryId) const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  ASSERT(HasDiffForUnsafe(countryId), ());
  return m_diffs.at(countryId);
}

bool Manager::HasDiffFor(storage::TCountryId const & countryId) const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return HasDiffForUnsafe(countryId);
}

bool Manager::HasDiffForUnsafe(storage::TCountryId const & countryId) const
{
  return m_diffs.find(countryId) != m_diffs.end();
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
