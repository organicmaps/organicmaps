#pragma once

#include <cstdint>
#include <ctime>
#include <functional>
#include <mutex>
#include <string>

namespace editor
{
class UserStats
{
public:
  UserStats();
  UserStats(time_t const updateTime, uint32_t const rating, uint32_t const changesCount,
            std::string const & levelUpFeat);

  bool IsValid() const { return m_valid; }

  operator bool() const { return IsValid(); }

  bool GetChangesCount(int32_t & changesCount) const;
  bool GetRank(int32_t & rank) const;
  bool GetLevelUpRequiredFeat(std::string & levelUpFeat) const;

  time_t GetLastUpdate() const { return m_updateTime; }

private:
  int32_t m_changesCount;
  int32_t m_rank;
  time_t m_updateTime;
  /// A very doubtful field representing what a user must commit to have a better rank.
  std::string m_levelUpRequiredFeat;
  bool m_valid;
};

class UserStatsLoader
{
public:
  using OnUpdateCallback = std::function<void()>;

  enum class UpdatePolicy { Lazy, Force };

  UserStatsLoader();

  /// Synchronously sends request to the server. Updates stats and returns true on success.
  bool Update(std::string const & userName);

  /// Launches the update process if stats are too old or if policy is UpdatePolicy::Force.
  /// The process posts fn to a gui thread on success.
  void Update(std::string const & userName, UpdatePolicy policy, OnUpdateCallback fn);
  /// Calls Update with UpdatePolicy::Lazy.
  void Update(std::string const & userName, OnUpdateCallback fn);

  /// Resets internal state and removes records from settings.
  void DropStats(std::string const & userName);

  /// Atomically returns stats if userName is still actual.
  UserStats GetStats(std::string const & userName) const;

  /// Debug only.
  std::string GetUserName() const;

private:
  /// Not thread-safe, but called only in constructor.
  bool LoadFromSettings();
  /// Not thread-safe, use synchonization.
  void SaveToSettings();
  void DropSettings();

  std::string m_userName;

  time_t m_lastUpdate;
  mutable std::mutex m_mutex;

  UserStats m_userStats;
};
}  // namespace editor
