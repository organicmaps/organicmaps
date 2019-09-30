#pragma once

#include "storage/diff_scheme/diff_types.hpp"
#include "storage/storage_defines.hpp"

#include "generator/mwm_diff/diff.hpp"

#include "base/observer_list.hpp"
#include "base/thread_checker.hpp"
#include "base/thread_pool_delayed.hpp"

#include <functional>
#include <mutex>
#include <string>
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
    std::string m_diffReadyPath;
    LocalFilePtr m_diffFile;
    LocalFilePtr m_oldMwmFile;
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

  // Checks whether the diff for |countryId| is available for download or
  // has been downloaded.
  bool HasDiffFor(storage::CountryId const & countryId) const;

  void MarkAsApplied(storage::CountryId const & countryId);
  void RemoveDiffForCountry(storage::CountryId const & countryId);
  void AbortDiffScheme();

  Status GetStatus() const;

  void Load(NameDiffInfoMap && info);
  static void ApplyDiff(ApplyDiffParams && p, base::Cancellable const & cancellable,
                        OnDiffApplicationFinished const & task);

private:
  template <typename Fn>
  bool WithNotAppliedDiff(storage::CountryId const & countryId, Fn && fn) const
  {
    if (m_status != Status::Available)
      return false;

    auto const it = m_diffs.find(countryId);
    if (it == m_diffs.cend() || it->second.m_isApplied)
      return false;

    fn(it->second);
    return true;
  }

  Status m_status = Status::Undefined;
  NameDiffInfoMap m_diffs;
};
}  // namespace diffs
}  // namespace storage
