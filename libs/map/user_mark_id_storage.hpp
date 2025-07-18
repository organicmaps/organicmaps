#pragma once

#include "map/user_mark.hpp"

#include "kml/types.hpp"

class UserMarkIdStorage
{
public:
  static UserMarkIdStorage & Instance();

  static UserMark::Type GetMarkType(kml::MarkId id);

  kml::MarkId GetNextUserMarkId(UserMark::Type type);
  kml::TrackId GetNextTrackId();
  kml::MarkGroupId GetNextCategoryId();

  bool CheckIds(kml::FileData const & fileData) const;

  void ResetBookmarkId();
  void ResetTrackId();
  void ResetCategoryId();

  void EnableSaving(bool enable);

private:
  UserMarkIdStorage();

  void LoadLastBookmarkId();
  void LoadLastTrackId();
  void LoadLastCategoryId();

  void SaveLastBookmarkId();
  void SaveLastTrackId();
  void SaveLastCategoryId();

  bool HasKey(std::string const & name);

  bool m_isJustCreated;

  uint64_t m_lastBookmarkId;
  uint64_t m_lastTrackId;
  uint64_t m_lastUserMarkId;
  uint64_t m_lastCategoryId;

  uint64_t m_initialLastBookmarkId;
  uint64_t m_initialLastTrackId;
  uint64_t m_initialLastCategoryId;

  bool m_savingEnabled = true;
};
