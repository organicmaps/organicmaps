#pragma once

#include "map/user_mark.hpp"

#include <atomic>

class UserMarkIdStorage
{
public:
  static UserMarkIdStorage & Instance();

  static UserMark::Type GetMarkType(kml::MarkId id);

  bool IsJustCreated() const;

  kml::MarkId GetNextUserMarkId(UserMark::Type type);
  kml::TrackId GetNextTrackId();
  kml::MarkGroupId GetNextCategoryId();

  void ResetBookmarkId();
  void ResetTrackId();
  void ResetCategoryId();

  // Disable saving. For test purpose only!
  void EnableTestMode(bool enable);

private:
  UserMarkIdStorage();

  void LoadLastBookmarkId();
  void LoadLastTrackId();
  void LoadLastCategoryId();

  void SaveLastBookmarkId();
  void SaveLastTrackId();
  void SaveLastCategoryId();

  bool HasKey(std::string const & name);

  std::atomic<bool> m_isJustCreated;

  std::atomic<uint64_t> m_lastBookmarkId;
  std::atomic<uint64_t> m_lastTrackId;
  std::atomic<uint64_t> m_lastUserMarkId;
  std::atomic<uint64_t> m_lastCategoryId;

  bool m_testModeEnabled = false;
};
