#import "ElevationProfileData.h"
#import "MWMTypes.h"

#include "map/elevation_info.hpp"
#include "map/gps_track.hpp"

NS_ASSUME_NONNULL_BEGIN

@interface ElevationProfileData (Core)

- (instancetype)initWithTrackId:(MWMTrackID)trackId elevationInfo:(ElevationInfo const &)elevationInfo;
- (nullable instancetype)initWithTrackId:(MWMTrackID)trackId
                           elevationInfo:(ElevationInfo const &)elevationInfo
                             isCancelled:(ElevationProfileCancellation)isCancelled;
- (nullable instancetype)initWithRecordingSnapshot:(GpsTrack::ElevationSnapshot const &)snapshot
                                       isCancelled:(ElevationProfileCancellation)isCancelled;
- (BOOL)matchesRecordingSnapshot:(GpsTrack::ElevationSnapshot const &)snapshot;

@end

NS_ASSUME_NONNULL_END
