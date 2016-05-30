#pragma once

#include "std/cstdint.hpp"
#include "std/string.hpp"

namespace editor
{
class UserStats
{
public:
  explicit UserStats(string const & userName);
  explicit UserStats(string const & userName, uint32_t rating, uint32_t changesCount);

  bool IsChangesCountInitialized() const;
  bool IsRankInitialized() const;

  int32_t GetChangesCount() const { return m_changesCount; }
  int32_t GetRank() const { return m_rank; }

  bool GetUpdateStatus() const { return m_updateStatus; }
  bool Update();

private:
  string m_userName;
  int32_t m_changesCount;
  int32_t m_rank;

  /// True if last update was successful.
  bool m_updateStatus;
};
}  // namespace editor
