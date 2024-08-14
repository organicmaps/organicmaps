#pragma once

#include "kml/types.hpp"
#include "kml/types_v8mm.hpp"

namespace kml
{
struct TrackDataV9MM
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(TrackDataV9MM, visitor(m_id, "id"),
                                  visitor(m_localId, "localId"),
                                  visitor(m_name, "name"),
                                  visitor(m_description, "description"),
                                  visitor(m_layers, "layers"),
                                  visitor(m_timestamp, "timestamp"),
                                  visitor(m_flag1, "flag1"), // Extra field introduced in V9MM.
                                  visitor(m_geometry, "geometry"),
                                  visitor(m_visible, "visible"),
                                  visitor(m_constant1, "constant1"),
                                  visitor(m_constant2, "constant2"),
                                  visitor(m_constant3, "constant3"),
                                  visitor(m_nearestToponyms, "nearestToponyms"),
                                  visitor(m_properties, "properties"),
                                  VISITOR_COLLECTABLE)

  DECLARE_COLLECTABLE(LocalizableStringIndex, m_name, m_description, m_nearestToponyms, m_properties)

  bool operator==(TrackDataV9MM const & data) const
  {
    return m_id == data.m_id && m_localId == data.m_localId && m_name == data.m_name &&
           m_description == data.m_description && m_layers == data.m_layers &&
           IsEqual(m_timestamp, data.m_timestamp) && m_geometry == data.m_geometry &&
           m_visible == data.m_visible && m_nearestToponyms == data.m_nearestToponyms &&
           m_properties == data.m_properties;
  }

  bool operator!=(TrackDataV9MM const & data) const { return !operator==(data); }

  TrackData ConvertToLatestVersion() const
  {
    TrackData data;
    data.m_id = m_id;
    data.m_localId = m_localId;
    data.m_name = m_name;
    data.m_description = m_description;
    data.m_layers = m_layers;
    data.m_timestamp = m_timestamp;
    data.m_geometry = m_geometry;
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
  TimestampMillis m_timestamp{};
  MultiGeometry m_geometry;
  // Visibility.
  bool m_visible = true;
  // These constants were introduced in KMB V8MM. Usually have value 0. Don't know its purpose.
  uint8_t m_constant1 = 0;
  uint8_t m_constant2 = 0;
  uint8_t m_constant3 = 0;
  // Nearest toponyms.
  std::vector<std::string> m_nearestToponyms;
  // Key-value properties.
  Properties m_properties;
  // Extra field introduced in V9MM.
  bool m_flag1 = true;
};


// FileDataV8MM contains the same sections as FileDataV8MM but with changed m_tracksData format
struct FileDataV9MM
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(FileDataV9MM, visitor(m_serverId, "serverId"),
                                  visitor(m_categoryData, "category"),
                                  visitor(m_bookmarksData, "bookmarks"),
                                  visitor(m_tracksData, "tracks"))

  bool operator==(FileDataV9MM const & data) const
  {
    return m_serverId == data.m_serverId && m_categoryData == data.m_categoryData &&
           m_bookmarksData == data.m_bookmarksData && m_tracksData == data.m_tracksData;
  }

  bool operator!=(FileDataV9MM const & data) const { return !operator==(data); }

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
    for (auto & t : m_tracksData)
      data.m_tracksData.emplace_back(t.ConvertToLatestVersion());

    return data;
  }

  // Device id (it will not be serialized in text files).
  std::string m_deviceId;
  // Server id.
  std::string m_serverId;
  // Category's data.
  CategoryDataV8MM m_categoryData;
  // Bookmarks collection.
  std::vector<BookmarkDataV8MM> m_bookmarksData;
  // Tracks collection.
  std::vector<TrackDataV9MM> m_tracksData;
};
}  // namespace kml
