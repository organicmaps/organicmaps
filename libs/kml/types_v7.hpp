#pragma once

#include "kml/type_utils.hpp"
#include "kml/types.hpp"
#include "kml/types_v8.hpp"

#include "base/visitor.hpp"

#include <cmath>
#include <ios>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace kml
{
// TODO(tomilov): check that BookmarkData changed in V8
struct BookmarkDataV7
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(BookmarkDataV7, visitor(m_id, "id"), visitor(m_name, "name"),
                                  visitor(m_description, "description"), visitor(m_featureTypes, "featureTypes"),
                                  visitor(m_customName, "customName"), visitor(m_color, "color"),
                                  visitor(m_icon, "icon"), visitor(m_viewportScale, "viewportScale"),
                                  visitor(m_timestamp, "timestamp"), visitor(m_point, "point"),
                                  visitor(m_boundTracks, "boundTracks"), visitor(m_visible, "visible"),
                                  visitor(m_nearestToponym, "nearestToponym"), visitor(m_properties, "properties"),
                                  VISITOR_COLLECTABLE)

  DECLARE_COLLECTABLE(LocalizableStringIndex, m_name, m_description, m_customName, m_nearestToponym, m_properties)

  bool operator==(BookmarkDataV7 const & data) const
  {
    return m_id == data.m_id && m_name == data.m_name && m_description == data.m_description &&
           m_color == data.m_color && m_icon == data.m_icon && m_viewportScale == data.m_viewportScale &&
           IsEqual(m_timestamp, data.m_timestamp) && m_point.EqualDxDy(data.m_point, kMwmPointAccuracy) &&
           m_featureTypes == data.m_featureTypes && m_customName == data.m_customName &&
           m_boundTracks == data.m_boundTracks && m_visible == data.m_visible &&
           m_nearestToponym == data.m_nearestToponym && m_properties == data.m_properties;
  }

  bool operator!=(BookmarkDataV7 const & data) const { return !operator==(data); }

  BookmarkData ConvertToLatestVersion()
  {
    BookmarkData data;
    data.m_id = m_id;
    data.m_name = m_name;
    data.m_description = m_description;
    data.m_featureTypes = m_featureTypes;
    data.m_customName = m_customName;
    data.m_color = m_color;
    data.m_icon = m_icon;
    data.m_viewportScale = m_viewportScale;
    data.m_timestamp = m_timestamp;
    data.m_point = m_point;
    data.m_boundTracks = m_boundTracks;
    data.m_visible = m_visible;
    data.m_nearestToponym = m_nearestToponym;
    data.m_properties = m_properties;
    return data;
  }

  // Unique id (it will not be serialized in text files).
  MarkId m_id = kInvalidMarkId;
  // Bookmark's name.
  LocalizableString m_name;
  // Bookmark's description.
  LocalizableString m_description;
  // Bound feature's types: type indices sorted by importance, the most
  // important one goes first.
  std::vector<uint32_t> m_featureTypes;
  // Custom bookmark's name.
  LocalizableString m_customName;
  // Bookmark's color.
  ColorData m_color;
  // Bookmark's icon.
  BookmarkIcon m_icon = BookmarkIcon::None;
  // Viewport scale. 0 is a default value (no scale set).
  uint8_t m_viewportScale = 0;
  // Creation timestamp.
  Timestamp m_timestamp = {};
  // Coordinates in mercator.
  m2::PointD m_point;
  // Bound tracks (vector contains local track ids).
  std::vector<LocalId> m_boundTracks;
  // Visibility.
  bool m_visible = true;
  // Nearest toponym.
  std::string m_nearestToponym;
  // Key-value properties.
  Properties m_properties;
};

using TrackDataV7 = TrackDataV8;

// TODO(tomilov): check that CategoryData changed in V8
struct CategoryDataV7
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(CategoryDataV7, visitor(m_id, "id"), visitor(m_name, "name"),
                                  visitor(m_imageUrl, "imageUrl"), visitor(m_annotation, "annotation"),
                                  visitor(m_description, "description"), visitor(m_visible, "visible"),
                                  visitor(m_authorName, "authorName"), visitor(m_authorId, "authorId"),
                                  visitor(m_rating, "rating"), visitor(m_reviewsNumber, "reviewsNumber"),
                                  visitor(m_lastModified, "lastModified"), visitor(m_accessRules, "accessRules"),
                                  visitor(m_tags, "tags"), visitor(m_toponyms, "toponyms"),
                                  visitor(m_languageCodes, "languageCodes"), visitor(m_properties, "properties"),
                                  VISITOR_COLLECTABLE)

  DECLARE_COLLECTABLE(LocalizableStringIndex, m_name, m_annotation, m_description, m_imageUrl, m_authorName, m_authorId,
                      m_tags, m_toponyms, m_properties)

  bool operator==(CategoryDataV7 const & data) const
  {
    double constexpr kEps = 1e-5;
    return m_id == data.m_id && m_name == data.m_name && m_imageUrl == data.m_imageUrl &&
           m_annotation == data.m_annotation && m_description == data.m_description && m_visible == data.m_visible &&
           m_accessRules == data.m_accessRules && m_authorName == data.m_authorName && m_authorId == data.m_authorId &&
           fabs(m_rating - data.m_rating) < kEps && m_reviewsNumber == data.m_reviewsNumber &&
           IsEqual(m_lastModified, data.m_lastModified) && m_tags == data.m_tags && m_toponyms == data.m_toponyms &&
           m_languageCodes == data.m_languageCodes && m_properties == data.m_properties;
  }

  bool operator!=(CategoryDataV7 const & data) const { return !operator==(data); }

  CategoryData ConvertToLatestVersion()
  {
    CategoryData data;
    data.m_id = m_id;
    data.m_name = m_name;
    data.m_imageUrl = m_imageUrl;
    data.m_annotation = m_annotation;
    data.m_description = m_description;
    data.m_visible = m_visible;
    data.m_authorName = m_authorName;
    data.m_authorId = m_authorId;
    data.m_lastModified = m_lastModified;
    data.m_rating = m_rating;
    data.m_reviewsNumber = m_reviewsNumber;
    data.m_accessRules = m_accessRules;
    data.m_tags = m_tags;
    data.m_toponyms = m_toponyms;
    data.m_languageCodes = m_languageCodes;
    data.m_properties = m_properties;
    return data;
  }

  // Unique id (it will not be serialized in text files).
  MarkGroupId m_id = kInvalidMarkGroupId;
  // Category's name.
  LocalizableString m_name;
  // Image URL.
  std::string m_imageUrl;
  // Category's description.
  LocalizableString m_annotation;
  // Category's description.
  LocalizableString m_description;
  // Collection visibility.
  bool m_visible = true;
  // Author's name.
  std::string m_authorName;
  // Author's id.
  std::string m_authorId;
  // Last modification timestamp.
  Timestamp m_lastModified;
  // Rating.
  double m_rating = 0.0;
  // Number of reviews.
  uint32_t m_reviewsNumber = 0;
  // Access rules.
  AccessRules m_accessRules = AccessRules::Local;
  // Collection of tags.
  std::vector<std::string> m_tags;
  // Collection of geo ids for relevant toponyms.
  std::vector<std::string> m_toponyms;
  // Language codes.
  std::vector<int8_t> m_languageCodes;
  // Key-value properties.
  Properties m_properties;
};

struct FileDataV7
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(FileDataV7, visitor(m_serverId, "serverId"), visitor(m_categoryData, "category"),
                                  visitor(m_bookmarksData, "bookmarks"), visitor(m_tracksData, "tracks"))

  bool operator==(FileDataV7 const & data) const
  {
    return m_serverId == data.m_serverId && m_categoryData == data.m_categoryData &&
           m_bookmarksData == data.m_bookmarksData && m_tracksData == data.m_tracksData;
  }

  bool operator!=(FileDataV7 const & data) const { return !operator==(data); }

  FileData ConvertToLatestVersion()
  {
    FileData data;
    data.m_deviceId = m_deviceId;
    data.m_serverId = m_serverId;

    data.m_categoryData = m_categoryData.ConvertToLatestVersion();

    data.m_bookmarksData.reserve(m_bookmarksData.size());
    for (auto & d : m_bookmarksData)
      data.m_bookmarksData.emplace_back(d.ConvertToLatestVersion());

    data.m_tracksData = m_tracksData;

    return data;
  }

  // Device id (it will not be serialized in text files).
  std::string m_deviceId;
  // Server id.
  std::string m_serverId;
  // Category's data.
  CategoryDataV7 m_categoryData;
  // Bookmarks collection.
  std::vector<BookmarkDataV7> m_bookmarksData;
  // Tracks collection.
  std::vector<TrackDataV7> m_tracksData;
};
}  // namespace kml
