#import "TrackInfo.h"

#include <CoreApi/Framework.h>
#include "map/track_statistics.hpp"

@interface TrackInfo (Core)

- (instancetype)initWithTrackStatistics:(TrackStatistics const &)statistics;

@end
