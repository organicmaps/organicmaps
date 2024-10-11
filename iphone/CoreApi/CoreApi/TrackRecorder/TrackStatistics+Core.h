#import "TrackStatistics.h"

#include <CoreApi/Framework.h>
#include "map/gps_track_collection.hpp"

@interface TrackStatistics (Core)

- (instancetype)initWithTrackData:(Track const *)track;
- (instancetype)initWithGpsTrackInfo:(GpsTrackCollection::GpsTrackInfo const &)info;

@end
