#include "storage/diff_scheme/diffs_data_source.hpp"

#include <algorithm>

namespace
{
bool IsDiffsAvailable(storage::diffs::NameDiffInfoMap const & diffs)
{
  return std::any_of(diffs.cbegin(), diffs.cend(), [](auto const & d) { return d.second.m_isApplied == false; });
}
}  // namespace

namespace storage
{
namespace diffs
{
void DiffsDataSource::SetDiffInfo(NameDiffInfoMap && info)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (info.empty())
  {
    m_status = Status::NotAvailable;
  }
  else
  {
    m_diffs = std::move(info);
    m_status = Status::Available;
  }
}

Status DiffsDataSource::GetStatus() const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  return m_status;
}

bool DiffsDataSource::SizeFor(storage::CountryId const & countryId, uint64_t & size) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (m_status != Status::Available)
    return false;

  auto const it = m_diffs.find(countryId);
  if (it == m_diffs.cend())
    return false;

  size = it->second.m_size;
  return true;
}

bool DiffsDataSource::SizeToDownloadFor(storage::CountryId const & countryId, uint64_t & size) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  return WithNotAppliedDiff(countryId, [&size](DiffInfo const & info) { size = info.m_size; });
}

bool DiffsDataSource::VersionFor(storage::CountryId const & countryId, uint64_t & v) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  return WithNotAppliedDiff(countryId, [&v](DiffInfo const & info) { v = info.m_version; });
}

bool DiffsDataSource::HasDiffFor(storage::CountryId const & countryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  return WithNotAppliedDiff(countryId, [](DiffInfo const &) {});
}

void DiffsDataSource::MarkAsApplied(storage::CountryId const & countryId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto it = m_diffs.find(countryId);
  if (it == m_diffs.end())
    return;

  it->second.m_isApplied = true;

  if (!IsDiffsAvailable(m_diffs))
    m_status = Status::NotAvailable;
}

void DiffsDataSource::RemoveDiffForCountry(storage::CountryId const & countryId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_diffs.erase(countryId);

  if (m_diffs.empty() || !IsDiffsAvailable(m_diffs))
    m_status = Status::NotAvailable;
}

void DiffsDataSource::AbortDiffScheme()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_status = Status::NotAvailable;
  m_diffs.clear();
}
}  // namespace diffs
}  // namespace storage
