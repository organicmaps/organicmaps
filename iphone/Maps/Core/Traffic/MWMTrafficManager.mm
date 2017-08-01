#import "MWMTrafficManager.h"
#import "MWMCommon.h"

#include "Framework.h"

#include "map/traffic_manager.hpp"

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
      self.state = state;
      for (TObserver observer in self.observers)
        [observer onTrafficStateUpdated];
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

+ (MWMTrafficManagerState)state
{
  switch ([MWMTrafficManager manager].state)
  {
  case TrafficManager::TrafficState::Disabled: return MWMTrafficManagerStateDisabled;
  case TrafficManager::TrafficState::Enabled: return MWMTrafficManagerStateEnabled;
  case TrafficManager::TrafficState::WaitingData: return MWMTrafficManagerStateWaitingData;
  case TrafficManager::TrafficState::Outdated: return MWMTrafficManagerStateOutdated;
  case TrafficManager::TrafficState::NoData: return MWMTrafficManagerStateNoData;
  case TrafficManager::TrafficState::NetworkError: return MWMTrafficManagerStateNetworkError;
  case TrafficManager::TrafficState::ExpiredData: return MWMTrafficManagerStateExpiredData;
  case TrafficManager::TrafficState::ExpiredApp: return MWMTrafficManagerStateExpiredApp;
  }
}

+ (void)enableTraffic:(BOOL)enable
{
  auto & f = GetFramework();
  f.GetTrafficManager().SetEnabled(enable);
  f.SaveTrafficEnabled(enable);
}

@end
