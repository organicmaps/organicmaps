#import "ElevationProfileData.h"

#include "map/elevation_info.hpp"

NS_ASSUME_NONNULL_BEGIN

@interface ElevationProfileData (Core)

- (instancetype)initWithElevationInfo:(ElevationInfo const &)elevationInfo
                             serverId:(NSString *)serverId
                          activePoint:(double)activePoint
                           myPosition:(double)myPosition;

@end

NS_ASSUME_NONNULL_END
