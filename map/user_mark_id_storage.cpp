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

PersistentIdStorage::PersistentIdStorage()
  : m_lastUserMarkId(0)
{
  LoadLastBookmarkId();
  LoadLastTrackId();
  LoadLastCategoryId();
}

// static
PersistentIdStorage & PersistentIdStorage::Instance()
{
  static PersistentIdStorage instance;
  return instance;
}

// static
UserMark::Type PersistentIdStorage::GetMarkType(kml::MarkId id)
{
  return static_cast<UserMark::Type>(id >> (sizeof(id) * 8 - kMarkIdTypeBitsCount));
}

void PersistentIdStorage::ResetBookmarkId()
{
  m_lastBookmarkId = 0;
  SaveLastBookmarkId();
}

void PersistentIdStorage::ResetTrackId()
{
  m_lastTrackId = 0;
  SaveLastTrackId();
}

void PersistentIdStorage::ResetCategoryId()
{
  m_lastCategoryId = static_cast<uint64_t>(UserMark::Type::USER_MARK_TYPES_COUNT_MAX);
  SaveLastCategoryId();
}

kml::MarkId PersistentIdStorage::GetNextUserMarkId(UserMark::Type type)
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

kml::TrackId PersistentIdStorage::GetNextTrackId()
{
  auto const id = static_cast<kml::TrackId>(++m_lastTrackId);
  SaveLastTrackId();
  return id;
}

kml::MarkGroupId PersistentIdStorage::GetNextCategoryId()
{
  auto const id = static_cast<kml::TrackId>(++m_lastCategoryId);
  SaveLastCategoryId();
  return id;
}

void PersistentIdStorage::LoadLastBookmarkId()
{
  uint64_t lastId;
  std::string val;
  if (GetPlatform().GetSecureStorage().Load(kLastBookmarkId, val) && strings::to_uint64(val, lastId))
    m_lastBookmarkId = lastId;
  else
    m_lastBookmarkId = 0;
}

void PersistentIdStorage::LoadLastTrackId()
{
  uint64_t lastId;
  std::string val;
  if (GetPlatform().GetSecureStorage().Load(kLastTrackId, val) && strings::to_uint64(val, lastId))
    m_lastTrackId = lastId;
  else
    m_lastTrackId = 0;
}

void PersistentIdStorage::LoadLastCategoryId()
{
  uint64_t lastId;
  std::string val;
  if (GetPlatform().GetSecureStorage().Load(kLastBookmarkCategoryId, val) && strings::to_uint64(val, lastId))
    m_lastCategoryId = std::max(static_cast<uint64_t>(UserMark::USER_MARK_TYPES_COUNT_MAX), lastId);
  else
    m_lastCategoryId = static_cast<uint64_t>(UserMark::USER_MARK_TYPES_COUNT_MAX);
}

void PersistentIdStorage::SaveLastBookmarkId()
{
  if (m_testModeEnabled)
    return;
  GetPlatform().GetSecureStorage().Save(kLastBookmarkId, strings::to_string(m_lastBookmarkId.load()));
}

void PersistentIdStorage::SaveLastTrackId()
{
  if (m_testModeEnabled)
    return;
  GetPlatform().GetSecureStorage().Save(kLastTrackId, strings::to_string(m_lastTrackId.load()));
}

void PersistentIdStorage::SaveLastCategoryId()
{
  if (m_testModeEnabled)
    return;
  GetPlatform().GetSecureStorage().Save(kLastBookmarkCategoryId, strings::to_string(m_lastCategoryId.load()));
}

void PersistentIdStorage::EnableTestMode(bool enable)
{
  m_testModeEnabled = enable;
}
