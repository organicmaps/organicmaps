#import "MWMMapOverlayManager.h"

#include "Framework.h"

@interface MWMMapOverlayManager ()

@property(nonatomic) NSHashTable<id<MWMMapOverlayManagerObserver>> *observers;

@property(nonatomic) TrafficManager::TrafficState trafficState;
@property(nonatomic) TransitReadManager::TransitSchemeState transitState;
@property(nonatomic) IsolinesManager::IsolinesState isolinesState;

@end

@implementation MWMMapOverlayManager

#pragma mark - Instance

+ (MWMMapOverlayManager *)manager {
  static MWMMapOverlayManager *manager;
  static dispatch_once_t onceToken = 0;
  dispatch_once(&onceToken, ^{
    manager = [[self alloc] initManager];
  });
  return manager;
}

- (instancetype)initManager {
  self = [super init];
  if (self) {
    _observers = [NSHashTable weakObjectsHashTable];
    GetFramework().GetTrafficManager().SetStateListener([self](TrafficManager::TrafficState state) {
      self.trafficState = state;
      for (id<MWMMapOverlayManagerObserver> observer in self.observers) {
        [observer onTrafficStateUpdated];
      }
    });
    GetFramework().GetTransitManager().SetStateListener([self](TransitReadManager::TransitSchemeState state) {
      self.transitState = state;
      for (id<MWMMapOverlayManagerObserver> observer in self.observers) {
        if ([observer respondsToSelector:@selector(onTransitStateUpdated)]) {
          [observer onTransitStateUpdated];
        }
      }
    });
    GetFramework().GetIsolinesManager().SetStateListener([self](IsolinesManager::IsolinesState state) {
      self.isolinesState = state;
      for (id<MWMMapOverlayManagerObserver> observer in self.observers) {
        if ([observer respondsToSelector:@selector(onIsoLinesStateUpdated)]) {
          [observer onIsoLinesStateUpdated];
        }
      }
    });
  }
  return self;
}

#pragma mark - Add/Remove Observers

+ (void)addObserver:(id<MWMMapOverlayManagerObserver>)observer {
  [[MWMMapOverlayManager manager].observers addObject:observer];
}

+ (void)removeObserver:(id<MWMMapOverlayManagerObserver>)observer {
  [[MWMMapOverlayManager manager].observers removeObject:observer];
}

+ (MWMMapOverlayTrafficState)trafficState {
  switch ([MWMMapOverlayManager manager].trafficState) {
    case TrafficManager::TrafficState::Disabled:
      return MWMMapOverlayTrafficStateDisabled;
    case TrafficManager::TrafficState::Enabled:
      return MWMMapOverlayTrafficStateEnabled;
    case TrafficManager::TrafficState::WaitingData:
      return MWMMapOverlayTrafficStateWaitingData;
    case TrafficManager::TrafficState::Outdated:
      return MWMMapOverlayTrafficStateOutdated;
    case TrafficManager::TrafficState::NoData:
      return MWMMapOverlayTrafficStateNoData;
    case TrafficManager::TrafficState::NetworkError:
      return MWMMapOverlayTrafficStateNetworkError;
    case TrafficManager::TrafficState::ExpiredData:
      return MWMMapOverlayTrafficStateExpiredData;
    case TrafficManager::TrafficState::ExpiredApp:
      return MWMMapOverlayTrafficStateExpiredApp;
  }
}

+ (MWMMapOverlayTransitState)transitState {
  switch ([MWMMapOverlayManager manager].transitState) {
    case TransitReadManager::TransitSchemeState::Disabled:
      return MWMMapOverlayTransitStateDisabled;
    case TransitReadManager::TransitSchemeState::Enabled:
      return MWMMapOverlayTransitStateEnabled;
    case TransitReadManager::TransitSchemeState::NoData:
      return MWMMapOverlayTransitStateNoData;
  }
}

+ (MWMMapOverlayIsolinesState)isolinesState {
  switch ([MWMMapOverlayManager manager].isolinesState) {
    case IsolinesManager::IsolinesState::Disabled:
      return MWMMapOverlayIsolinesStateDisabled;
    case IsolinesManager::IsolinesState::Enabled:
      return MWMMapOverlayIsolinesStateEnabled;
    case IsolinesManager::IsolinesState::ExpiredData:
      return MWMMapOverlayIsolinesStateExpiredData;
    case IsolinesManager::IsolinesState::NoData:
      return MWMMapOverlayIsolinesStateNoData;
  }
}

+ (BOOL)trafficEnabled {
  return [MWMMapOverlayManager manager].trafficState != TrafficManager::TrafficState::Disabled;
}

+ (BOOL)transitEnabled {
  return [MWMMapOverlayManager manager].transitState != TransitReadManager::TransitSchemeState::Disabled;
}

+ (BOOL)isoLinesEnabled {
  return [MWMMapOverlayManager manager].isolinesState != IsolinesManager::IsolinesState::Disabled;
}

+ (void)setTrafficEnabled:(BOOL)enable {
  if (enable) {
    [self setTransitEnabled:false];
    [self setIsoLinesEnabled:false];
  }

  auto &f = GetFramework();
  f.GetTrafficManager().SetEnabled(enable);
  f.SaveTrafficEnabled(enable);
}

+ (void)setTransitEnabled:(BOOL)enable {
  if (enable) {
    [self setTrafficEnabled:!enable];
    [self setIsoLinesEnabled:false];
  }

  auto &f = GetFramework();
  f.GetTransitManager().EnableTransitSchemeMode(enable);
  f.SaveTransitSchemeEnabled(enable);
}

+ (void)setIsoLinesEnabled:(BOOL)enable {
  if (enable) {
    [self setTrafficEnabled:false];
    [self setTransitEnabled:false];
  }

  auto &f = GetFramework();
  f.GetIsolinesManager().SetEnabled(enable);
  f.SaveIsolonesEnabled(enable);
}

@end
