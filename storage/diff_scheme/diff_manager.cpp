#include "storage/diff_scheme/diff_manager.hpp"

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
void Manager::Load(NameDiffInfoMap && info)
{
  if (info.empty())
  {
    m_status = Status::NotAvailable;

    alohalytics::Stats::Instance().LogEvent("Downloader_DiffScheme_OnStart_fallback");
  }
  else
  {
    m_diffs = std::move(info);
    m_status = Status::Available;
  }
}

// static
void Manager::ApplyDiff(ApplyDiffParams && p, base::Cancellable const & cancellable,
                        Manager::OnDiffApplicationFinished const & task)
{
  using namespace generator::mwm_diff;

  GetPlatform().RunTask(Platform::Thread::File, [p = std::move(p), &cancellable, task] {
    CHECK(p.m_diffFile, ());
    CHECK(p.m_oldMwmFile, ());

    auto & diffReadyPath = p.m_diffReadyPath;
    auto & diffFile = p.m_diffFile;
    auto const diffPath = diffFile->GetPath(MapFileType::Diff);
    auto result = DiffApplicationResult::Failed;

    diffFile->SyncWithDisk();

    auto const isOnDisk = diffFile->OnDisk(MapFileType::Diff);
    auto const isFilePrepared = isOnDisk || base::RenameFileX(diffReadyPath, diffPath);

    if (isFilePrepared)
    {
      // Sync with disk after renaming.
      if (!isOnDisk)
        diffFile->SyncWithDisk();

      std::string const oldMwmPath = p.m_oldMwmFile->GetPath(MapFileType::Map);
      std::string const newMwmPath = diffFile->GetPath(MapFileType::Map);
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
      diffFile->DeleteFromDisk(MapFileType::Diff);
      break;
    case DiffApplicationResult::Cancelled:
      // The diff file will be deleted by storage.
      // Another way would be to leave it on disk but all consequences
      // of interacting with storage are much harder to be taken into account that way.
      break;
    case DiffApplicationResult::Failed:
      diffFile->DeleteFromDisk(MapFileType::Diff);
      alohalytics::Stats::Instance().LogEvent(
          "Downloader_DiffScheme_error",
          {{"type", "patching"},
           {"error", isFilePrepared ? "Cannot apply diff" : "Cannot prepare file"}});
      break;
    }

    GetPlatform().RunTask(Platform::Thread::Gui, [task, result]()
    {
      task(result);
    });
  });
}

Status Manager::GetStatus() const
{
  return m_status;
}

bool Manager::SizeFor(storage::CountryId const & countryId, uint64_t & size) const
{
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
  auto it = m_diffs.find(countryId);
  if (it == m_diffs.end())
    return;

  it->second.m_isApplied = true;

  if (!IsDiffsAvailable(m_diffs))
    m_status = Status::NotAvailable;
}

void Manager::RemoveDiffForCountry(storage::CountryId const & countryId)
{
  m_diffs.erase(countryId);

  if (m_diffs.empty() || !IsDiffsAvailable(m_diffs))
    m_status = Status::NotAvailable;
}

void Manager::AbortDiffScheme()
{
  m_status = Status::NotAvailable;
  m_diffs.clear();
}
}  // namespace diffs
}  // namespace storage
