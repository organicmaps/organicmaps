#pragma once

#include "std/cstdint.hpp"
#include "std/ctime.hpp"
#include "std/function.hpp"
#include "std/mutex.hpp"
#include "std/string.hpp"

namespace editor
{
class UserStats
{
public:
  UserStats();
  UserStats(time_t const updateTime, uint32_t const rating,
            uint32_t const changesCount, string const & levelUpFeat);

  bool IsValid() const { return m_valid; }

  operator bool() const { return IsValid(); }

  bool GetChangesCount(int32_t & changesCount) const;
  bool GetRank(int32_t & rank) const;
  bool GetLevelUpRequiredFeat(string & levelUpFeat) const;

  time_t GetLastUpdate() const { return m_updateTime; }

private:
  int32_t m_changesCount;
  int32_t m_rank;
  time_t m_updateTime;
  /// A very doubtful field representing what a user must commit to have a better rank.
  string m_levelUpRequiredFeat;
  bool m_valid;
};

class UserStatsLoader
{
public:
  using TOnUpdateCallback = function<void()>;

  enum class UpdatePolicy { Lazy, Force };

  UserStatsLoader();

  /// Synchronously sends request to the server. Updates stats and returns true on success.
  bool Update(string const & userName);

  /// Launches the update process if stats are too old or if policy is UpdatePolicy::Force.
  /// The process posts fn to a gui thread on success.
  void Update(string const & userName, UpdatePolicy policy, TOnUpdateCallback fn);
  /// Calls Update with UpdatePolicy::Lazy.
  void Update(string const & userName, TOnUpdateCallback fn);

  /// Resets internal state and removes records from settings.
  void DropStats(string const & userName);

  /// Atomically returns stats if userName is still actual.
  UserStats GetStats(string const & userName) const;

  /// Debug only.
  string GetUserName() const;

private:
  /// Not thread-safe, but called only in constructor.
  bool LoadFromSettings();
  /// Not thread-safe, use synchonization.
  void SaveToSettings();
  void DropSettings();

  string m_userName;

  time_t m_lastUpdate;
  mutable mutex m_mutex;

  UserStats m_userStats;
};
}  // namespace editor
