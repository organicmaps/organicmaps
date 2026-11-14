#pragma once

#include "kml/types.hpp"

#include <string>
#include <vector>

class BookmarkManager;

// Stable source keys let an explicit re-import update ordinary bookmarks without duplicating them.
struct BookmarkImportEntry
{
  std::string m_key;
  std::string m_name;
  std::string m_address;
  double m_lat = 0;
  double m_lon = 0;
  std::string m_customImage = {};
  bool m_poiStyle = false;
  kml::MarkGroupId m_groupId = kml::kInvalidMarkGroupId;
  // Retire only unchanged entries in the saved bookmark's source category, after a successful upsert.
  std::vector<std::string> m_supersededKeys = {};
  // Opt-in protection for one-shot sources whose moved/edited bookmarks are now user-owned.
  bool m_requireSourceCategory = false;
  // Readback eligibility; rechecked natively at commit time.
  bool m_canModify = true;
  // Opaque source input identity, saved atomically with the resolved point.
  std::string m_sourceFingerprint = {};
};

struct BookmarkImportResult
{
  kml::MarkGroupId m_categoryId = kml::kInvalidMarkGroupId;
  size_t m_added = 0;
  size_t m_updated = 0;
  size_t m_unchanged = 0;
  size_t m_edited = 0;
  size_t m_skipped = 0;
};

// Main-thread operation. Preserves local changes, unrelated bookmarks, and category names/visibility.
std::vector<BookmarkImportEntry> GetImportedBookmarks(BookmarkManager const & manager, std::string const & source);

BookmarkImportResult ImportBookmarks(BookmarkManager & manager, std::string const & source,
                                     std::string const & categoryName,
                                     std::vector<BookmarkImportEntry> const & entries);
