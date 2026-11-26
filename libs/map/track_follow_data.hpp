#pragma once

#include "map/track.hpp"

#include "routing/track_router.hpp"

#include <optional>

// Resolve the selected source occurrence using both its coordinate and finite chart distance.
std::optional<TrackSourcePosition> ResolveTrackEntry(Track const & track, m2::PointD const & selectedPoint,
                                                     double distanceFromBeginningM);

// The caller supplies a finite distance in [0, track length]. A gap uses the preceding section.
TrackSourcePosition ResolveTrackEntryAt(Track const & track, double distanceFromBeginningM);

// Closest real segment within the profile's matching corridor. Exact ties use source order.
std::optional<TrackSourcePosition> FindNearbyTrackEntry(Track const & track, m2::PointD const & fix, double thresholdM);

// A map hit can choose a distinct pass. Where the hit coincides with the current visit keep that occurrence.
// distanceM retains the tap's source distance before endpoint normalization.
std::optional<TrackSourcePosition> FindTappedTrackEntry(Track const & track, m2::RectD const & rect,
                                                        TrackSourcePosition const & current, double windowM,
                                                        double & distanceM);

// Find the physical join near the selected source occurrence. Without a GPS fix only exact overlap qualifies.
std::optional<TrackSourcePosition> ProjectSelectedTrackVisit(Track const & track, TrackSourcePosition const & selection,
                                                             TrackSourcePosition const & nearest,
                                                             m2::PointD const * fix, double thresholdM);

// Both ends of this section must be plausible visits, and the projection must be on a terminal portion.
std::optional<TrackSourcePosition> GetFreshLoopEntry(Track const & track, TrackSourcePosition const & entry,
                                                     TrackSourcePosition const & nearest, bool reverse,
                                                     m2::PointD const & fix, double thresholdM);

// Translate confirmed distance along the directed polyline back to the original source occurrence.
// Mercator segment lengths are additive even where clipping or projected stops split a sparse edge.
TrackSourcePosition AdvanceTrackEntry(Track const & track, TrackSourcePosition entry, bool reverse,
                                      double distanceMerc);

// Whether horizontal geometry remains in the requested direction from a valid source position, without preparing stops.
bool HasRemainingTrackGeometry(kml::MultiGeometry const & geometry, TrackSourcePosition const & entry,
                               bool reverse = false);

// Reverse from the same entry when usable. Near a loop trailhead start a full lap at the current destination.
std::optional<TrackSourcePosition> GetReverseTrackEntry(Track const & track, TrackSourcePosition const & entry,
                                                        TrackSourcePosition const & destination);

// Prepare the connected remainder and its stops in travel order, preserving altitude availability.
// Returns nullopt for an empty remainder, or writes the reached source endpoint if requested.
std::optional<routing::TrackFollowingData> BuildTrackFollowingData(kml::TrackData const & source,
                                                                   TrackSourcePosition const & entry,
                                                                   bool reverse = false,
                                                                   TrackSourcePosition * destination = nullptr);
