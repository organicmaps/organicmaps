#import "MWMTrafficManagerObserver.h"

#include "map/traffic_manager.hpp"

@interface MWMTrafficManager : NSObject

+ (void)addObserver:(id<MWMTrafficManagerObserver>)observer;
+ (void)removeObserver:(id<MWMTrafficManagerObserver>)observer;

+ (TrafficManager::TrafficState)state;

+ (void)enableTraffic:(BOOL)enable;

@end
