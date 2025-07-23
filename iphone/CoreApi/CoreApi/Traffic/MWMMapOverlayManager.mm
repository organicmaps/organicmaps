#import "MWMMapOverlayManager.h"

#include "Framework.h"

static NSString * kGuidesWasShown = @"guidesWasShown";
static NSString * didChangeOutdoorMapStyle = @"didChangeOutdoorMapStyle";

@interface MWMMapOverlayManager ()

@property(nonatomic) NSHashTable<id<MWMMapOverlayManagerObserver>> * observers;

@end

@implementation MWMMapOverlayManager

#pragma mark - Instance

+ (MWMMapOverlayManager *)manager
{
  static MWMMapOverlayManager * manager;
  static dispatch_once_t onceToken = 0;
  dispatch_once(&onceToken, ^{ manager = [[self alloc] initManager]; });
  return manager;
}

- (instancetype)initManager
{
  self = [super init];
  if (self)
  {
    _observers = [NSHashTable weakObjectsHashTable];
    GetFramework().GetTrafficManager().SetStateListener([self](TrafficManager::TrafficState state)
    {
      for (id<MWMMapOverlayManagerObserver> observer in self.observers)
        if ([observer respondsToSelector:@selector(onTrafficStateUpdated)])
          [observer onTrafficStateUpdated];
    });
    GetFramework().GetTransitManager().SetStateListener([self](TransitReadManager::TransitSchemeState state)
    {
      for (id<MWMMapOverlayManagerObserver> observer in self.observers)
        if ([observer respondsToSelector:@selector(onTransitStateUpdated)])
          [observer onTransitStateUpdated];
    });
    GetFramework().GetIsolinesManager().SetStateListener([self](IsolinesManager::IsolinesState state)
    {
      for (id<MWMMapOverlayManagerObserver> observer in self.observers)
        if ([observer respondsToSelector:@selector(onIsoLinesStateUpdated)])
          [observer onIsoLinesStateUpdated];
    });
    [NSNotificationCenter.defaultCenter addObserverForName:didChangeOutdoorMapStyle
                                                    object:nil
                                                     queue:nil
                                                usingBlock:^(NSNotification * _Nonnull notification) {
                                                  for (id<MWMMapOverlayManagerObserver> observer in self.observers)
                                                    if ([observer respondsToSelector:@selector(onOutdoorStateUpdated)])
                                                      [observer onOutdoorStateUpdated];
                                                }];
  }
  return self;
}

#pragma mark - Add/Remove Observers

+ (void)addObserver:(id<MWMMapOverlayManagerObserver>)observer
{
  [[MWMMapOverlayManager manager].observers addObject:observer];
}

+ (void)removeObserver:(id<MWMMapOverlayManagerObserver>)observer
{
  [[MWMMapOverlayManager manager].observers removeObject:observer];
}

#pragma mark - Properties

+ (MWMMapOverlayTrafficState)trafficState
{
  switch (GetFramework().GetTrafficManager().GetState())
  {
  case TrafficManager::TrafficState::Disabled: return MWMMapOverlayTrafficStateDisabled;
  case TrafficManager::TrafficState::Enabled: return MWMMapOverlayTrafficStateEnabled;
  case TrafficManager::TrafficState::WaitingData: return MWMMapOverlayTrafficStateWaitingData;
  case TrafficManager::TrafficState::Outdated: return MWMMapOverlayTrafficStateOutdated;
  case TrafficManager::TrafficState::NoData: return MWMMapOverlayTrafficStateNoData;
  case TrafficManager::TrafficState::NetworkError: return MWMMapOverlayTrafficStateNetworkError;
  case TrafficManager::TrafficState::ExpiredData: return MWMMapOverlayTrafficStateExpiredData;
  case TrafficManager::TrafficState::ExpiredApp: return MWMMapOverlayTrafficStateExpiredApp;
  }
}

+ (MWMMapOverlayTransitState)transitState
{
  switch (GetFramework().GetTransitManager().GetState())
  {
  case TransitReadManager::TransitSchemeState::Disabled: return MWMMapOverlayTransitStateDisabled;
  case TransitReadManager::TransitSchemeState::Enabled: return MWMMapOverlayTransitStateEnabled;
  case TransitReadManager::TransitSchemeState::NoData: return MWMMapOverlayTransitStateNoData;
  }
}

+ (MWMMapOverlayIsolinesState)isolinesState
{
  switch (GetFramework().GetIsolinesManager().GetState())
  {
  case IsolinesManager::IsolinesState::Disabled: return MWMMapOverlayIsolinesStateDisabled;
  case IsolinesManager::IsolinesState::Enabled: return MWMMapOverlayIsolinesStateEnabled;
  case IsolinesManager::IsolinesState::ExpiredData: return MWMMapOverlayIsolinesStateExpiredData;
  case IsolinesManager::IsolinesState::NoData: return MWMMapOverlayIsolinesStateNoData;
  }
}

+ (MWMMapOverlayOutdoorState)outdoorState
{
  switch (GetFramework().GetMapStyle())
  {
  case MapStyleOutdoorsLight:
  case MapStyleOutdoorsDark: return MWMMapOverlayOutdoorStateEnabled;
  default: return MWMMapOverlayOutdoorStateDisabled;
  }
}

+ (BOOL)trafficEnabled
{
  return self.trafficState != MWMMapOverlayTrafficStateDisabled;
}

+ (BOOL)transitEnabled
{
  return self.transitState != MWMMapOverlayTransitStateDisabled;
}

+ (BOOL)isoLinesEnabled
{
  return self.isolinesState != MWMMapOverlayIsolinesStateDisabled;
}

+ (BOOL)isolinesVisible
{
  return GetFramework().GetIsolinesManager().IsVisible();
}

+ (BOOL)outdoorEnabled
{
  return self.outdoorState != MWMMapOverlayOutdoorStateDisabled;
}

+ (void)setTrafficEnabled:(BOOL)enable
{
  if (enable)
  {
    [self setTransitEnabled:false];
    [self setIsoLinesEnabled:false];
  }

  auto & f = GetFramework();
  f.GetTrafficManager().SetEnabled(enable);
  f.SaveTrafficEnabled(enable);
}

+ (void)setTransitEnabled:(BOOL)enable
{
  if (enable)
  {
    [self setTrafficEnabled:!enable];
    [self setIsoLinesEnabled:false];
    [self setOutdoorEnabled:false];
  }

  auto & f = GetFramework();
  f.GetTransitManager().EnableTransitSchemeMode(enable);
  f.SaveTransitSchemeEnabled(enable);
}

+ (void)setIsoLinesEnabled:(BOOL)enable
{
  if (enable)
  {
    [self setTrafficEnabled:false];
    [self setTransitEnabled:false];
  }

  auto & f = GetFramework();
  f.GetIsolinesManager().SetEnabled(enable);
  f.SaveIsolinesEnabled(enable);
}

+ (void)setOutdoorEnabled:(BOOL)enable
{
  if (enable)
  {
    [self setTransitEnabled:false];
    [self setTrafficEnabled:false];
  }

  auto & f = GetFramework();
  switch (f.GetMapStyle())
  {
  case MapStyleDefaultLight:
  case MapStyleVehicleLight:
  case MapStyleOutdoorsLight: f.SetMapStyle(enable ? MapStyleOutdoorsLight : MapStyleDefaultLight); break;
  case MapStyleDefaultDark:
  case MapStyleVehicleDark:
  case MapStyleOutdoorsDark: f.SetMapStyle(enable ? MapStyleOutdoorsDark : MapStyleDefaultDark); break;
  default: break;
  }

  // TODO: - Observing for the selected/deselected state of the Outdoor style should be implemented not by
  // NSNotificationCenter but the same way as for IsoLines with 'GetFramework().GetIsolinesManager().SetStateListener'.
  [NSNotificationCenter.defaultCenter postNotificationName:didChangeOutdoorMapStyle object:nil];
}

@end
