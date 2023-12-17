#include "map/user_mark_id_storage.hpp"

#include "platform/platform.hpp"

#include "base/string_utils.hpp"

namespace
{
uint32_t const kMarkIdTypeBitsCount = 4;
std::string const kLastBookmarkId = "LastBookmarkId";
std::string const kLastTrackId = "LastTrackId";
std::string const kLastBookmarkCategoryId = "LastBookmarkCategoryId";
}  // namespace

UserMarkIdStorage::UserMarkIdStorage()
  : m_lastUserMarkId(0)
{
  m_isJustCreated = !(HasKey(kLastBookmarkCategoryId) && HasKey(kLastBookmarkId) &&
                      HasKey(kLastTrackId));
  if (m_isJustCreated)
  {
    ResetCategoryId();
    ResetBookmarkId();
    ResetTrackId();
  }
  else
  {
    LoadLastBookmarkId();
    LoadLastTrackId();
    LoadLastCategoryId();
  }

  m_initialLastBookmarkId = m_lastBookmarkId;
  m_initialLastTrackId = m_lastTrackId;
  m_initialLastCategoryId = m_lastCategoryId;
}

// static
UserMarkIdStorage & UserMarkIdStorage::Instance()
{
  static UserMarkIdStorage instance;
  return instance;
}

// static
UserMark::Type UserMarkIdStorage::GetMarkType(kml::MarkId id)
{
  return static_cast<UserMark::Type>(id >> (sizeof(id) * 8 - kMarkIdTypeBitsCount));
}

void UserMarkIdStorage::ResetBookmarkId()
{
  m_lastBookmarkId = 0;
  SaveLastBookmarkId();
}

void UserMarkIdStorage::ResetTrackId()
{
  m_lastTrackId = 0;
  SaveLastTrackId();
}

void UserMarkIdStorage::ResetCategoryId()
{
  m_lastCategoryId = static_cast<uint64_t>(UserMark::Type::USER_MARK_TYPES_COUNT_MAX);
  SaveLastCategoryId();
}

bool UserMarkIdStorage::CheckIds(kml::FileData const & fileData) const
{
  // Storage is just created. Check failed.
  if (m_isJustCreated)
    return false;

  // There are ids of categories, bookmarks or tracks with values
  // more than last stored maximums. Check failed.
  if (fileData.m_categoryData.m_id != kml::kInvalidMarkGroupId &&
      fileData.m_categoryData.m_id > m_initialLastCategoryId)
  {
    return false;
  }

  for (auto const & b : fileData.m_bookmarksData)
  {
    if (b.m_id != kml::kInvalidMarkId && b.m_id > m_initialLastBookmarkId)
      return false;
  }

  for (auto const & t : fileData.m_tracksData)
  {
    if (t.m_id != kml::kInvalidTrackId && t.m_id > m_initialLastTrackId)
      return false;
  }

  for (auto const & c : fileData.m_compilationsData)
  {
    if (c.m_id != kml::kInvalidMarkGroupId && c.m_id > m_initialLastCategoryId)
      return false;
  }

  // No one corner case. Check passed.
  return true;
}

kml::MarkId UserMarkIdStorage::GetNextUserMarkId(UserMark::Type type)
{
  static_assert(UserMark::Type::USER_MARK_TYPES_COUNT <= (1 << kMarkIdTypeBitsCount),
                "Not enough bits for user mark type.");

  auto const typeBits = static_cast<uint64_t>(type) << (sizeof(kml::MarkId) * 8 - kMarkIdTypeBitsCount);
  if (type == UserMark::Type::BOOKMARK)
  {
    auto const id = static_cast<kml::MarkId>((++m_lastBookmarkId) | typeBits);
    SaveLastBookmarkId();
    return id;
  }
  else
  {
    return static_cast<kml::MarkId>((++m_lastUserMarkId) | typeBits);
  }
}

kml::TrackId UserMarkIdStorage::GetNextTrackId()
{
  auto const id = static_cast<kml::TrackId>(++m_lastTrackId);
  SaveLastTrackId();
  return id;
}

kml::MarkGroupId UserMarkIdStorage::GetNextCategoryId()
{
  auto const id = static_cast<kml::MarkGroupId>(++m_lastCategoryId);
  SaveLastCategoryId();
  return id;
}

bool UserMarkIdStorage::HasKey(std::string const & name)
{
  std::string val;
  return GetPlatform().GetSecureStorage().Load(name, val);
}

void UserMarkIdStorage::LoadLastBookmarkId()
{
  uint64_t lastId;
  std::string val;
  if (GetPlatform().GetSecureStorage().Load(kLastBookmarkId, val) && strings::to_uint64(val, lastId))
    m_lastBookmarkId = lastId;
  else
    m_lastBookmarkId = 0;
}

void UserMarkIdStorage::LoadLastTrackId()
{
  uint64_t lastId;
  std::string val;
  if (GetPlatform().GetSecureStorage().Load(kLastTrackId, val) && strings::to_uint64(val, lastId))
    m_lastTrackId = lastId;
  else
    m_lastTrackId = 0;
}

void UserMarkIdStorage::LoadLastCategoryId()
{
  uint64_t lastId;
  std::string val;
  if (GetPlatform().GetSecureStorage().Load(kLastBookmarkCategoryId, val) && strings::to_uint64(val, lastId))
    m_lastCategoryId = std::max(static_cast<uint64_t>(UserMark::USER_MARK_TYPES_COUNT_MAX), lastId);
  else
    m_lastCategoryId = static_cast<uint64_t>(UserMark::USER_MARK_TYPES_COUNT_MAX);
}

void UserMarkIdStorage::SaveLastBookmarkId()
{
  if (!m_savingEnabled)
    return;
  GetPlatform().GetSecureStorage().Save(kLastBookmarkId, strings::to_string(m_lastBookmarkId));
}

void UserMarkIdStorage::SaveLastTrackId()
{
  if (!m_savingEnabled)
    return;
  GetPlatform().GetSecureStorage().Save(kLastTrackId, strings::to_string(m_lastTrackId));
}

void UserMarkIdStorage::SaveLastCategoryId()
{
  if (!m_savingEnabled)
    return;
  GetPlatform().GetSecureStorage().Save(kLastBookmarkCategoryId, strings::to_string(m_lastCategoryId));
}

void UserMarkIdStorage::EnableSaving(bool enable)
{
  if (m_savingEnabled == enable)
    return;

  m_savingEnabled = enable;
  if (enable)
  {
    SaveLastBookmarkId();
    SaveLastTrackId();
    SaveLastCategoryId();
  }
}
