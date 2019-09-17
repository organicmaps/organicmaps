#include "storage/diff_scheme/diff_manager.hpp"
#include "storage/diff_scheme/diff_scheme_checker.hpp"

#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

#include "base/assert.hpp"
#include "base/cancellable.hpp"

#include <algorithm>

#include "3party/Alohalytics/src/alohalytics.h"

namespace
{
bool IsDiffsAvailable(storage::diffs::NameDiffInfoMap const & diffs)
{
  return std::any_of(diffs.cbegin(), diffs.cend(),
                     [](auto const & d) { return d.second.m_isApplied == false; });
}
}  // namespace

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

  m_workerThread.Push([this, localMapsInfo] {
    NameDiffInfoMap diffs = Checker::Check(localMapsInfo);

    std::lock_guard<std::mutex> lock(m_mutex);

    m_diffs = std::move(diffs);
    if (m_diffs.empty())
    {
      m_status = Status::NotAvailable;

      alohalytics::Stats::Instance().LogEvent("Downloader_DiffScheme_OnStart_fallback");
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

void Manager::ApplyDiff(ApplyDiffParams && p, base::Cancellable const & cancellable,
                        Manager::OnDiffApplicationFinished const & task)
{
  using namespace generator::mwm_diff;

  m_workerThread.Push([this, p, &cancellable, task] {
    CHECK(p.m_diffFile, ());
    CHECK(p.m_oldMwmFile, ());

    auto & diffReadyPath = p.m_diffReadyPath;
    auto & diffFile = p.m_diffFile;
    auto const diffPath = diffFile->GetPath(MapOptions::Diff);
    auto result = DiffApplicationResult::Failed;

    diffFile->SyncWithDisk();

    auto const isOnDisk = diffFile->OnDisk(MapOptions::Diff);
    auto const isFilePrepared = isOnDisk || base::RenameFileX(diffReadyPath, diffPath);

    if (isFilePrepared)
    {
      // Sync with disk after renaming.
      if (!isOnDisk)
        diffFile->SyncWithDisk();

      std::string const oldMwmPath = p.m_oldMwmFile->GetPath(MapOptions::Map);
      std::string const newMwmPath = diffFile->GetPath(MapOptions::Map);
      std::string const diffApplyingInProgressPath = newMwmPath + DIFF_APPLYING_FILE_EXTENSION;

      result = generator::mwm_diff::ApplyDiff(oldMwmPath, diffApplyingInProgressPath, diffPath,
                                              cancellable);
      if (result == DiffApplicationResult::Ok &&
          !base::RenameFileX(diffApplyingInProgressPath, newMwmPath))
      {
        result = DiffApplicationResult::Failed;
      }

      Platform::RemoveFileIfExists(diffApplyingInProgressPath);

      if (result != DiffApplicationResult::Ok)
        Platform::RemoveFileIfExists(newMwmPath);
    }

    switch (result)
    {
    case DiffApplicationResult::Ok:
    {
      diffFile->DeleteFromDisk(MapOptions::Diff);
      break;
    }
    case DiffApplicationResult::Cancelled:
      // The diff file will be deleted by storage.
      // Another way would be to leave it on disk but all consequences
      // of interacting with storage are much harder to be taken into account that way.
      break;
    case DiffApplicationResult::Failed:
    {
      diffFile->DeleteFromDisk(MapOptions::Diff);
      alohalytics::Stats::Instance().LogEvent(
          "Downloader_DiffScheme_error",
          {{"type", "patching"},
           {"error", isFilePrepared ? "Cannot apply diff" : "Cannot prepare file"}});

      std::lock_guard<std::mutex> lock(m_mutex);
      m_status = Status::NotAvailable;
      break;
    }
    }

    task(result);
  });
}

Status Manager::GetStatus() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_status;
}

bool Manager::SizeFor(storage::CountryId const & countryId, uint64_t & size) const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_status != Status::Available)
    return false;

  auto const it = m_diffs.find(countryId);
  if (it == m_diffs.cend())
    return false;

  size = it->second.m_size;
  return true;
}

bool Manager::SizeToDownloadFor(storage::CountryId const & countryId, uint64_t & size) const
{
  return WithNotAppliedDiff(countryId, [&size](DiffInfo const & info) { size = info.m_size; });
}

bool Manager::VersionFor(storage::CountryId const & countryId, uint64_t & v) const
{
  return WithNotAppliedDiff(countryId, [&v](DiffInfo const & info) { v = info.m_version; });
}

bool Manager::HasDiffFor(storage::CountryId const & countryId) const
{
  return WithNotAppliedDiff(countryId, [](DiffInfo const &) {});
}

void Manager::MarkAsApplied(storage::CountryId const & countryId)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_diffs.find(countryId);
  if (it == m_diffs.end())
    return;

  it->second.m_isApplied = true;

  if (!IsDiffsAvailable(m_diffs))
    m_status = Status::NotAvailable;
}

void Manager::RemoveDiffForCountry(storage::CountryId const & countryId)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_diffs.erase(countryId);

  if (m_diffs.empty() || !IsDiffsAvailable(m_diffs))
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
    auto const it = m_diffs.find(nameVersion.first);
    if (it == m_diffs.cend() || it->second.m_isApplied)
      return false;
  }
  return true;
}
}  // namespace diffs
}  // namespace storage
