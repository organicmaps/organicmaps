#pragma once

#include "routing/track_router.hpp"

#include "kml/types.hpp"

#include <optional>
#include <string>

struct TrackSourcePosition
{
  size_t m_line = 0;
  size_t m_segment = 0;
  double m_fraction = 0.0;

  bool operator==(TrackSourcePosition const &) const = default;
};

/// \brief Why a follow-track request was refused. Everything but Ok leaves the routing session
/// exactly as it was.
enum class FollowTrackResult
{
  Ok,
  /// The requested live source track does not exist.
  NoTrack,
  /// Active navigation or a pending Start prevents this operation.
  Busy,
  /// The selected occurrence is missing or no longer matches its source.
  InvalidSelection,
  /// No horizontal geometry remains in the selected connected section.
  NoRemainingGeometry,
};

std::string DebugPrint(FollowTrackResult result);

// Resolve the selected source occurrence using both its coordinate and finite chart distance.
std::optional<TrackSourcePosition> ResolveTrackEntry(kml::MultiGeometry const & geometry,
                                                     m2::PointD const & selectedPoint, double distanceFromBeginningM);

// The caller supplies a finite distance in [0, source length], summed like Track::GetLengthMeters().
TrackSourcePosition ResolveTrackEntryAt(kml::MultiGeometry const & geometry, double distanceFromBeginningM);

// Whether horizontal geometry remains after a valid source position, without preparing stops.
bool HasRemainingTrackGeometry(kml::MultiGeometry const & geometry, TrackSourcePosition const & entry);

// Prepare one connected remainder from a valid source position, preserving source order,
// stops and altitude availability. Returns nullopt when no horizontal geometry remains.
std::optional<routing::TrackFollowingData> BuildTrackFollowingData(kml::TrackData const & source,
                                                                   TrackSourcePosition const & entry);
