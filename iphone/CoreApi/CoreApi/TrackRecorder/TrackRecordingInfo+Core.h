#import "TrackRecordingInfo.h"

#include <CoreApi/Framework.h>
#include "map/gps_track_collection.hpp"

@interface TrackRecordingInfo (Core)

- (instancetype)initWithGpsTrackInfo:(GpsTrackInfo const &)info;

@end
