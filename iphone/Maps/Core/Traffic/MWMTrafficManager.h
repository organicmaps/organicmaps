#import "MWMTrafficManagerObserver.h"
#import "MWMTrafficManagerState.h"

@interface MWMTrafficManager : NSObject

+ (void)addObserver:(id<MWMTrafficManagerObserver>)observer;
+ (void)removeObserver:(id<MWMTrafficManagerObserver>)observer;

+ (MWMTrafficManagerState)state;

+ (void)enableTraffic:(BOOL)enable;

@end
