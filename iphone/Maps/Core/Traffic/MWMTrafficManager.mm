#import "MWMTrafficManager.h"
#import "MWMCommon.h"

#include "Framework.h"

namespace
{
using TObserver = id<MWMTrafficManagerObserver>;
using TObservers = NSHashTable<__kindof TObserver>;
}  // namespace

@interface MWMTrafficManager ()

@property(nonatomic) TObservers * observers;

@property(nonatomic) TrafficManager::TrafficState state;

@end

@implementation MWMTrafficManager

#pragma mark - Instance

+ (MWMTrafficManager *)manager
{
  static MWMTrafficManager * manager;
  static dispatch_once_t onceToken = 0;
  dispatch_once(&onceToken, ^{
    manager = [[super alloc] initManager];
  });
  return manager;
}

- (instancetype)initManager
{
  self = [super init];
  if (self)
  {
    _observers = [TObservers weakObjectsHashTable];
    auto & m = GetFramework().GetTrafficManager();
    m.SetStateListener([self](TrafficManager::TrafficState state) {
      runAsyncOnMainQueue(^{
        self.state = state;
        for (TObserver observer in self.observers)
          [observer onTrafficStateUpdated];
      });
    });
  }
  return self;
}

#pragma mark - Add/Remove Observers

+ (void)addObserver:(TObserver)observer
{
  [[MWMTrafficManager manager].observers addObject:observer];
}

+ (void)removeObserver:(TObserver)observer
{
  [[MWMTrafficManager manager].observers removeObject:observer];
}

+ (TrafficManager::TrafficState)state { return [MWMTrafficManager manager].state; }
+ (void)enableTraffic:(BOOL)enable
{
  auto & f = GetFramework();
  f.GetTrafficManager().SetEnabled(enable);
  f.SaveTrafficEnabled(enable);
}

@end
