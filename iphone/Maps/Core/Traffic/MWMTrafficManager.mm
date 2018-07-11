#import "MWMTrafficManager.h"

#include "Framework.h"

namespace
{
using Observer = id<MWMTrafficManagerObserver>;
using Observers = NSHashTable<Observer>;
}  // namespace

@interface MWMTrafficManager ()

@property(nonatomic) Observers * observers;

@property(nonatomic) TrafficManager::TrafficState trafficState;
@property(nonatomic) TransitReadManager::TransitSchemeState transitState;

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
    _observers = [Observers weakObjectsHashTable];
    auto & m = GetFramework().GetTrafficManager();
    m.SetStateListener([self](TrafficManager::TrafficState state) {
      self.trafficState = state;
      for (Observer observer in self.observers)
        [observer onTrafficStateUpdated];
    });
    GetFramework().GetTransitManager().SetStateListener([self](TransitReadManager::TransitSchemeState state) {
      self.transitState = state;
      for (Observer observer in self.observers)
      {
        if ([observer respondsToSelector:@selector(onTransitStateUpdated)])
          [observer onTransitStateUpdated];
      }
    });
  }
  return self;
}

#pragma mark - Add/Remove Observers

+ (void)addObserver:(Observer)observer
{
  [[MWMTrafficManager manager].observers addObject:observer];
}

+ (void)removeObserver:(Observer)observer
{
  [[MWMTrafficManager manager].observers removeObject:observer];
}

+ (MWMTrafficManagerState)trafficState
{
  switch ([MWMTrafficManager manager].trafficState)
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

+ (MWMTransitManagerState)transitState
{
  switch ([MWMTrafficManager manager].transitState)
  {
    case TransitReadManager::TransitSchemeState::Disabled: return MWMTransitManagerStateDisabled;
    case TransitReadManager::TransitSchemeState::Enabled: return MWMTransitManagerStateEnabled;
    case TransitReadManager::TransitSchemeState::NoData: return MWMTransitManagerStateNoData;
  }
}

+ (BOOL)trafficEnabled
{
  return [MWMTrafficManager manager].trafficState != TrafficManager::TrafficState::Disabled;
}

+ (BOOL)transitEnabled
{
  return [MWMTrafficManager manager].transitState != TransitReadManager::TransitSchemeState::Disabled;
}

+ (void)setTrafficEnabled:(BOOL)enable
{
  if (enable)
    [self setTransitEnabled:!enable];
  
  auto & f = GetFramework();
  f.GetTrafficManager().SetEnabled(enable);
  f.SaveTrafficEnabled(enable);
}

+ (void)setTransitEnabled:(BOOL)enable
{
  if (enable)
    [self setTrafficEnabled:!enable];
  
  auto & f = GetFramework();
  f.GetTransitManager().EnableTransitSchemeMode(enable);
  f.SaveTransitSchemeEnabled(enable);
}

@end
