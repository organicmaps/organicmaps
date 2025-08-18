#pragma once

#include "kml/type_utils.hpp"
#include "kml/types.hpp"
#include "kml/types_v7.hpp"

#include "base/visitor.hpp"

#include <cmath>
#include <ios>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace kml
{
using BookmarkDataV6 = BookmarkDataV7;

// All kml structures for V6 and V7 are same except TrackData.
// Saved V6 version of TrackData to support migration from V6 to V7.
struct TrackDataV6
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(TrackDataV6, visitor(m_id, "id"), visitor(m_localId, "localId"),
                                  visitor(m_name, "name"), visitor(m_description, "description"),
                                  visitor(m_layers, "layers"), visitor(m_timestamp, "timestamp"),
                                  visitor(m_points, "points"), visitor(m_visible, "visible"),
                                  visitor(m_nearestToponyms, "nearestToponyms"), visitor(m_properties, "properties"),
                                  VISITOR_COLLECTABLE)

  DECLARE_COLLECTABLE(LocalizableStringIndex, m_name, m_description, m_nearestToponyms, m_properties)

  bool operator==(TrackDataV6 const & data) const
  {
    return m_id == data.m_id && m_localId == data.m_localId && m_name == data.m_name &&
           m_description == data.m_description && m_layers == data.m_layers && IsEqual(m_timestamp, data.m_timestamp) &&
           IsEqual(m_points, data.m_points) && m_visible == data.m_visible &&
           m_nearestToponyms == data.m_nearestToponyms && m_properties == data.m_properties;
  }

  bool operator!=(TrackDataV6 const & data) const { return !operator==(data); }

  TrackData ConvertToLatestVersion()
  {
    TrackData data;
    data.m_id = m_id;
    data.m_localId = m_localId;
    data.m_name = m_name;
    data.m_description = m_description;
    data.m_layers = m_layers;
    data.m_timestamp = m_timestamp;
    data.m_geometry.FromPoints(m_points);
    data.m_visible = m_visible;
    data.m_nearestToponyms = m_nearestToponyms;
    data.m_properties = m_properties;

    return data;
  }

  // Unique id (it will not be serialized in text files).
  TrackId m_id = kInvalidTrackId;
  // Local track id.
  LocalId m_localId = 0;
  // Track's name.
  LocalizableString m_name;
  // Track's description.
  LocalizableString m_description;
  // Layers.
  std::vector<TrackLayer> m_layers;
  // Creation timestamp.
  Timestamp m_timestamp = {};
  // Points.
  std::vector<m2::PointD> m_points;
  // Visibility.
  bool m_visible = true;
  // Nearest toponyms.
  std::vector<std::string> m_nearestToponyms;
  // Key-value properties.
  Properties m_properties;
};

using CategoryDataV6 = CategoryDataV7;

struct FileDataV6
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(FileDataV6, visitor(m_serverId, "serverId"), visitor(m_categoryData, "category"),
                                  visitor(m_bookmarksData, "bookmarks"), visitor(m_tracksData, "tracks"))

  bool operator==(FileDataV6 const & data) const
  {
    return m_serverId == data.m_serverId && m_categoryData == data.m_categoryData &&
           m_bookmarksData == data.m_bookmarksData && m_tracksData == data.m_tracksData;
  }

  bool operator!=(FileDataV6 const & data) const { return !operator==(data); }

  FileData ConvertToLatestVersion()
  {
    FileData data;
    data.m_deviceId = m_deviceId;
    data.m_serverId = m_serverId;

    data.m_categoryData = m_categoryData.ConvertToLatestVersion();

    data.m_bookmarksData.reserve(m_bookmarksData.size());
    for (auto & d : m_bookmarksData)
      data.m_bookmarksData.emplace_back(d.ConvertToLatestVersion());

    data.m_tracksData.reserve(m_tracksData.size());
    for (auto & track : m_tracksData)
      data.m_tracksData.emplace_back(track.ConvertToLatestVersion());

    return data;
  }

  // Device id (it will not be serialized in text files).
  std::string m_deviceId;
  // Server id.
  std::string m_serverId;
  // Category's data.
  CategoryDataV7 m_categoryData;
  // Bookmarks collection.
  std::vector<BookmarkDataV6> m_bookmarksData;
  // Tracks collection.
  std::vector<TrackDataV6> m_tracksData;
};
}  // namespace kml
