#pragma once

#include "map/user_mark.hpp"

#include "kml/types.hpp"

#include <atomic>

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

  bool m_isJustCreated;

  //TODO: do we really need atomics here?
  std::atomic<uint64_t> m_lastBookmarkId;
  std::atomic<uint64_t> m_lastTrackId;
  std::atomic<uint64_t> m_lastUserMarkId;
  std::atomic<uint64_t> m_lastCategoryId;

  uint64_t m_initialLastBookmarkId;
  uint64_t m_initialLastTrackId;
  uint64_t m_initialLastCategoryId;

  bool m_testModeEnabled = false;
};
