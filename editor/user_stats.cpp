#include "editor/user_stats.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "coding/url.hpp"

#include "base/logging.hpp"
#include "base/thread.hpp"
#include "base/timer.hpp"

#include "3party/pugixml/src/pugixml.hpp"

namespace
{
std::string const kUserStatsUrl = "https://editor-api.maps.me/user?format=xml";
int32_t constexpr kUninitialized = -1;

auto constexpr kSettingsUserName = "LastLoggedUser";
auto constexpr kSettingsRating = "UserEditorRating";
auto constexpr kSettingsChangesCount = "UserEditorChangesCount";
auto constexpr kSettingsLastUpdate = "UserEditorLastUpdate";

auto constexpr kSecondsInHour = 60 * 60;
}  // namespace

namespace editor
{
// UserStat ----------------------------------------------------------------------------------------

UserStats::UserStats()
  : m_changesCount(kUninitialized), m_rank(kUninitialized)
  , m_updateTime(base::SecondsSinceEpochToTimeT(0)), m_valid(false)
{
}

UserStats::UserStats(time_t const updateTime, uint32_t const rating, uint32_t const changesCount,
                     std::string const & levelUpFeat)
  : m_changesCount(changesCount)
  , m_rank(rating)
  , m_updateTime(updateTime)
  , m_levelUpRequiredFeat(levelUpFeat)
  , m_valid(true)
{
}

bool UserStats::GetChangesCount(int32_t & changesCount) const
{
  if (m_changesCount == kUninitialized)
    return false;
  changesCount = m_changesCount;
  return true;
}

bool UserStats::GetRank(int32_t & rank) const
{
  if (m_rank == kUninitialized)
    return false;
  rank = m_rank;
  return true;
}

bool UserStats::GetLevelUpRequiredFeat(std::string & levelUpFeat) const
{
  if (m_levelUpRequiredFeat.empty())
    return false;
  levelUpFeat = m_levelUpRequiredFeat;
  return true;
}

// UserStatsLoader ---------------------------------------------------------------------------------

UserStatsLoader::UserStatsLoader()
  : m_lastUpdate(base::SecondsSinceEpochToTimeT(0))
{
  if (!LoadFromSettings())
    LOG(LINFO, ("There is no cached user stats info in settings"));
  else
    LOG(LINFO, ("User stats info was loaded successfully"));
}

bool UserStatsLoader::Update(std::string const & userName)
{
  if (userName.empty())
    return false;

  {
    std::lock_guard<std::mutex> g(m_mutex);
    m_userName = userName;
  }

  auto const url = kUserStatsUrl + "&name=" + url::UrlEncode(userName);
  platform::HttpClient request(url);
  request.SetRawHeader("User-Agent", GetPlatform().GetAppUserAgent());

  if (!request.RunHttpRequest())
  {
    LOG(LWARNING, ("Network error while connecting to", url));
    return false;
  }

  if (request.ErrorCode() != 200)
  {
    LOG(LWARNING, ("Server returned", request.ErrorCode(), "for url", url));
    return false;
  }

  auto const response = request.ServerResponse();

  pugi::xml_document document;
  if (!document.load_buffer(response.data(), response.size()))
  {
    LOG(LWARNING, ("Cannot parse server response:", response));
    return false;
  }

  auto changesCount = document.select_node("mmwatch/edits/@value").attribute().as_int(-1);
  auto rank = document.select_node("mmwatch/rank/@value").attribute().as_int(-1);
  auto levelUpFeat = document.select_node("mmwatch/levelUpFeat/@value").attribute().as_string();

  std::lock_guard<std::mutex> g(m_mutex);
  if (m_userName != userName)
    return false;

  m_lastUpdate = time(nullptr);
  m_userStats = UserStats(m_lastUpdate, rank, changesCount, levelUpFeat);
  SaveToSettings();

  return true;
}

void UserStatsLoader::Update(std::string const & userName, UpdatePolicy const policy,
                             OnUpdateCallback fn)
{
  auto nothingToUpdate = false;
  if (policy == UpdatePolicy::Lazy)
  {
    std::lock_guard<std::mutex> g(m_mutex);
    nothingToUpdate = m_userStats && m_userName == userName &&
                      difftime(time(nullptr), m_lastUpdate) < kSecondsInHour;
  }

  if (nothingToUpdate)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, fn);
    return;
  }

  GetPlatform().RunTask(Platform::Thread::Network, [this, userName, fn]
  {
    if (Update(userName))
      GetPlatform().RunTask(Platform::Thread::Gui, fn);
  });
}

void UserStatsLoader::Update(std::string const & userName, OnUpdateCallback fn)
{
  Update(userName, UpdatePolicy::Lazy, fn);
}

void UserStatsLoader::DropStats(std::string const & userName)
{
  std::lock_guard<std::mutex> g(m_mutex);
  if (m_userName != userName)
    return;
  m_userStats = {};
  DropSettings();
}

UserStats UserStatsLoader::GetStats(std::string const & userName) const
{
  std::lock_guard<std::mutex> g(m_mutex);
  if (m_userName == userName)
    return m_userStats;
  return {};
}

std::string UserStatsLoader::GetUserName() const
{
  std::lock_guard<std::mutex> g(m_mutex);
  return m_userName;
}

bool UserStatsLoader::LoadFromSettings()
{
  uint32_t rating = 0;
  uint32_t changesCount = 0;
  uint64_t lastUpdate = 0;

  if (!settings::Get(kSettingsUserName, m_userName) ||
      !settings::Get(kSettingsChangesCount, changesCount) ||
      !settings::Get(kSettingsRating, rating) ||
      !settings::Get(kSettingsLastUpdate, lastUpdate))
  {
    return false;
  }

  m_lastUpdate = base::SecondsSinceEpochToTimeT(lastUpdate);
  m_userStats = UserStats(m_lastUpdate, rating, changesCount, "");
  return true;
}

void UserStatsLoader::SaveToSettings()
{
  if (!m_userStats)
    return;

  settings::Set(kSettingsUserName, m_userName);
  int32_t rank;
  if (m_userStats.GetRank(rank))
    settings::Set(kSettingsRating, rank);
  int32_t changesCount;
  if (m_userStats.GetChangesCount(changesCount))
    settings::Set(kSettingsChangesCount, changesCount);
  settings::Set(kSettingsLastUpdate, base::TimeTToSecondsSinceEpoch(m_lastUpdate));
  // Do not save m_requiredLevelUpFeat for it becomes obsolete very fast.
}

void UserStatsLoader::DropSettings()
{
  settings::Delete(kSettingsUserName);
  settings::Delete(kSettingsRating);
  settings::Delete(kSettingsChangesCount);
  settings::Delete(kSettingsLastUpdate);
}
}  // namespace editor
