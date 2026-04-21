#pragma once

#include "kml/types.hpp"
#include "kml/types_v8mm.hpp"

#include "base/logging.hpp"

#include <ctime>

namespace kml
{

MultiGeometry mergeGeometry(std::vector<MultiGeometry> && aGeometries);

// Per-point capture timestamps introduced in the V11 MapsMe track format.
// On disk: varuint count, then N varuints, each encoding (ms_since_epoch << 7) | low7_flags.
// Kept in memory as seconds-since-epoch time_t to match MultiGeometry::TimeT.
struct TrackPointTimestamps
{
  bool operator==(TrackPointTimestamps const & rhs) const { return m_values == rhs.m_values; }

  std::vector<time_t> m_values;
};

inline std::string DebugPrint(TrackPointTimestamps const & pts)
{
  return ::DebugPrint(pts.m_values);
}

struct TrackDataV9MM : TrackDataV8MM
{
  // The V11 MapsMe format drops m_constant3 and appends a per-point timestamps vector
  // between m_constant2 and the collectionIndex. Older V9MM files where m_constant3 == 0
  // read back as an empty TrackPointTimestamps (varuint count 0), keeping backward compat.
  DECLARE_VISITOR_AND_DEBUG_PRINT(
      TrackDataV9MM, visitor(m_id, "id"), visitor(m_localId, "localId"), visitor(m_name, "name"),
      visitor(m_description, "description"), visitor(m_layers, "layers"), visitor(m_timestamp, "timestamp"),
      visitor(m_multiGeometry, "multiGeometry"),  // V9MM introduced multiGeometry instead of a single one
      visitor(m_visible, "visible"), visitor(m_constant1, "constant1"), visitor(m_constant2, "constant2"),
      visitor(m_pointTimestamps, "pointTimestamps"),  // V11: per-point capture times (supersedes m_constant3).
      visitor(m_nearestToponyms, "nearestToponyms"), visitor(m_properties, "properties"), VISITOR_COLLECTABLE)

  DECLARE_COLLECTABLE(LocalizableStringIndex, m_name, m_description, m_nearestToponyms, m_properties)

  TrackData ConvertToLatestVersion()
  {
    TrackData data;
    data.m_id = m_id;
    data.m_localId = m_localId;
    data.m_name = m_name;
    data.m_description = m_description;
    data.m_layers = m_layers;
    data.m_timestamp = m_timestamp;
    data.m_geometry = mergeGeometry(std::move(m_multiGeometry));

    // MultiGeometry's invariant (see MultiGeometry::IsValid) requires m_timestamps.size()
    // to match m_lines.size(). V9MM's MultiGeometry visitor only populates m_lines, so
    // pad m_timestamps here so downstream consumers (e.g. SaveTrackGeometry) don't trip
    // on the size mismatch.
    data.m_geometry.m_timestamps.resize(data.m_geometry.m_lines.size());

    // V11 per-point timestamps arrive as a single flat vector covering every point across
    // all lines of a gx:MultiTrack. Split it back by line length so the per-line invariant
    // timestamps[i].size() == lines[i].size() holds downstream.
    if (!m_pointTimestamps.m_values.empty())
    {
      size_t totalPoints = 0;
      for (auto const & line : data.m_geometry.m_lines)
        totalPoints += line.size();

      if (totalPoints == m_pointTimestamps.m_values.size())
      {
        size_t offset = 0;
        for (size_t i = 0; i < data.m_geometry.m_lines.size(); ++i)
        {
          auto const sz = data.m_geometry.m_lines[i].size();
          data.m_geometry.m_timestamps[i].assign(m_pointTimestamps.m_values.begin() + offset,
                                                 m_pointTimestamps.m_values.begin() + offset + sz);
          offset += sz;
        }
      }
      else
      {
        LOG(LWARNING, ("V9MM track point timestamps count", m_pointTimestamps.m_values.size(),
                       "doesn't match total points count", totalPoints, "- dropping timestamps"));
      }
    }

    data.m_visible = m_visible;
    data.m_nearestToponyms = m_nearestToponyms;
    data.m_properties = m_properties;
    return data;
  }

  std::vector<MultiGeometry> m_multiGeometry;
  TrackPointTimestamps m_pointTimestamps;
};

// Contains the same sections as FileDataV8MM but with changed m_tracksData format
using FileDataV9MM = FileDataMMImpl<TrackDataV9MM>;

}  // namespace kml
