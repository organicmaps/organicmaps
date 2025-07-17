#pragma once

#include "storage/diff_scheme/diff_types.hpp"
#include "storage/storage_defines.hpp"

#include "base/thread_checker.hpp"

#include <memory>

namespace storage
{
namespace diffs
{
class DiffsDataSource;
using DiffsSourcePtr = std::shared_ptr<diffs::DiffsDataSource>;

class DiffsDataSource final
{
public:
  void SetDiffInfo(NameDiffInfoMap && info);
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

  ThreadChecker m_threadChecker;

  Status m_status = Status::NotAvailable;
  NameDiffInfoMap m_diffs;
};
}  // namespace diffs
}  // namespace storage
