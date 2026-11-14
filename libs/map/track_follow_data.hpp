#pragma once

#include "routing/track_router.hpp"

#include "kml/types.hpp"

#include <expected>
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
  /// The track does not exist (anymore).
  NoTrack,
  /// The track has no two distinct points to follow.
  DegenerateTrack,
  /// A profile a track can never be followed with (Transit, Ruler).
  UnsupportedProfile,
  /// Called before RoutingManager::Init().
  NotInitialized,
  Busy,
  InvalidSelection,
  NoRemainingGeometry,
};

std::string DebugPrint(FollowTrackResult result);

// Resolve the selected source occurrence using both its coordinate and distance.
std::optional<TrackSourcePosition> ResolveTrackEntry(kml::MultiGeometry const & geometry,
                                                     m2::PointD const & selectedPoint, double distanceFromBeginningM);

// Resolve a finite chart distance clamped to the full source length.
TrackSourcePosition ResolveTrackEntryAt(kml::MultiGeometry const & geometry, double distanceFromBeginningM);

// Prepare the connected remainder in travel order, retaining stops and altitude availability.
// Reverse follows the part before the entry back to its connected start.
std::expected<routing::TrackFollowingData, FollowTrackResult> BuildTrackFollowingData(kml::TrackData const & source,
                                                                                      TrackSourcePosition const & entry,
                                                                                      bool reverse);
