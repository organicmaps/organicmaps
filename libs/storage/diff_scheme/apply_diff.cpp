#include "storage/diff_scheme/apply_diff.hpp"

#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

#include "base/assert.hpp"
#include "base/cancellable.hpp"

namespace storage
{
namespace diffs
{
void ApplyDiff(ApplyDiffParams && p, base::Cancellable const & cancellable, OnDiffApplicationFinished const & task)
{
  using namespace generator::mwm_diff;

  GetPlatform().RunTask(Platform::Thread::File, [p = std::move(p), &cancellable, task]
  {
    CHECK(p.m_diffFile, ());
    CHECK(p.m_oldMwmFile, ());

    auto & diffReadyPath = p.m_diffReadyPath;
    auto & diffFile = p.m_diffFile;
    auto const diffPath = diffFile->GetPath(MapFileType::Diff);
    auto result = DiffApplicationResult::Failed;

    diffFile->SyncWithDisk();
    if (!diffFile->OnDisk(MapFileType::Diff))
    {
      base::RenameFileX(diffReadyPath, diffPath);
      diffFile->SyncWithDisk();
    }

    auto const isFilePrepared = diffFile->OnDisk(MapFileType::Diff);

    if (isFilePrepared)
    {
      std::string const oldMwmPath = p.m_oldMwmFile->GetPath(MapFileType::Map);
      std::string const newMwmPath = diffFile->GetPath(MapFileType::Map);
      std::string const diffApplyingInProgressPath = newMwmPath + DIFF_APPLYING_FILE_EXTENSION;

      result = generator::mwm_diff::ApplyDiff(oldMwmPath, diffApplyingInProgressPath, diffPath, cancellable);
      if (result == DiffApplicationResult::Ok && !base::RenameFileX(diffApplyingInProgressPath, newMwmPath))
        result = DiffApplicationResult::Failed;

      Platform::RemoveFileIfExists(diffApplyingInProgressPath);

      if (result != DiffApplicationResult::Ok)
        Platform::RemoveFileIfExists(newMwmPath);
    }

    switch (result)
    {
    case DiffApplicationResult::Ok: diffFile->DeleteFromDisk(MapFileType::Diff); break;
    case DiffApplicationResult::Cancelled:
      // The diff file will be deleted by storage.
      // Another way would be to leave it on disk but all consequences
      // of interacting with storage are much harder to be taken into account that way.
      break;
    case DiffApplicationResult::Failed: diffFile->DeleteFromDisk(MapFileType::Diff); break;
    }

    GetPlatform().RunTask(Platform::Thread::Gui, [task, result]() { task(result); });
  });
}
}  // namespace diffs
}  // namespace storage
