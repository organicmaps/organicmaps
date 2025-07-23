#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"

#include <CoreApi/Framework.h>

#include "platform/downloader_defines.hpp"

namespace
{
using Observer = id<MWMFrameworkObserver>;
using TRouteBuildingObserver = id<MWMFrameworkRouteBuilderObserver>;
using TDrapeObserver = id<MWMFrameworkDrapeObserver>;

using Observers = NSHashTable<Observer>;

Protocol * pRouteBuildingObserver = @protocol(MWMFrameworkRouteBuilderObserver);
Protocol * pDrapeObserver = @protocol(MWMFrameworkDrapeObserver);

using TLoopBlock = void (^)(__kindof Observer observer);

void loopWrappers(Observers * observers, TLoopBlock block)
{
  dispatch_async(dispatch_get_main_queue(), ^{
    for (Observer observer in observers)
      if (observer)
        block(observer);
  });
}
}  // namespace

@interface MWMFrameworkListener ()

@property(nonatomic) Observers * routeBuildingObservers;
@property(nonatomic) Observers * drapeObservers;

@end

@implementation MWMFrameworkListener

+ (MWMFrameworkListener *)listener
{
  static MWMFrameworkListener * listener;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{ listener = [[super alloc] initListener]; });
  return listener;
}

+ (void)addObserver:(Observer)observer
{
  dispatch_async(dispatch_get_main_queue(), ^{
    MWMFrameworkListener * listener = [MWMFrameworkListener listener];
    if ([observer conformsToProtocol:pRouteBuildingObserver])
      [listener.routeBuildingObservers addObject:observer];
    if ([observer conformsToProtocol:pDrapeObserver])
      [listener.drapeObservers addObject:observer];
  });
}

+ (void)removeObserver:(Observer)observer
{
  dispatch_async(dispatch_get_main_queue(), ^{
    MWMFrameworkListener * listener = [MWMFrameworkListener listener];
    [listener.routeBuildingObservers removeObject:observer];
    [listener.drapeObservers removeObject:observer];
  });
}

- (instancetype)initListener
{
  self = [super init];
  if (self)
  {
    _routeBuildingObservers = [Observers weakObjectsHashTable];
    _drapeObservers = [Observers weakObjectsHashTable];

    [self registerRouteBuilderListener];
    [self registerDrapeObserver];
  }
  return self;
}

#pragma mark - MWMFrameworkRouteBuilderObserver

- (void)registerRouteBuilderListener
{
  using namespace routing;
  using namespace storage;
  Observers * observers = self.routeBuildingObservers;
  auto & rm = GetFramework().GetRoutingManager();
  rm.SetRouteBuildingListener([observers](RouterResultCode code, CountriesSet const & absentCountries)
  {
    loopWrappers(observers, [code, absentCountries](TRouteBuildingObserver observer)
    { [observer processRouteBuilderEvent:code countries:absentCountries]; });
  });
  rm.SetRouteProgressListener([observers](float progress)
  {
    loopWrappers(observers, [progress](TRouteBuildingObserver observer)
    {
      if ([observer respondsToSelector:@selector(processRouteBuilderProgress:)])
        [observer processRouteBuilderProgress:progress];
    });
  });
  rm.SetRouteRecommendationListener([observers](RoutingManager::Recommendation recommendation)
  {
    MWMRouterRecommendation rec;
    switch (recommendation)
    {
    case RoutingManager::Recommendation::RebuildAfterPointsLoading:
      rec = MWMRouterRecommendationRebuildAfterPointsLoading;
      break;
    }
    loopWrappers(observers, [rec](TRouteBuildingObserver observer)
    {
      if ([observer respondsToSelector:@selector(processRouteRecommendation:)])
        [observer processRouteRecommendation:rec];
    });
  });
  rm.SetRouteSpeedCamShowListener([observers](m2::PointD const & point, double cameraSpeedKmPH)
  {
    loopWrappers(observers, [cameraSpeedKmPH](TRouteBuildingObserver observer)
    {
      if ([observer respondsToSelector:@selector(speedCameraShowedUpOnRoute:)])
        [observer speedCameraShowedUpOnRoute:cameraSpeedKmPH];
    });
  });
  rm.SetRouteSpeedCamsClearListener([observers]()
  {
    loopWrappers(observers, ^(TRouteBuildingObserver observer) {
      if ([observer respondsToSelector:@selector(speedCameraLeftVisibleArea)])
        [observer speedCameraLeftVisibleArea];
    });
  });
}

#pragma mark - MWMFrameworkDrapeObserver

- (void)registerDrapeObserver
{
  Observers * observers = self.drapeObservers;
  auto & f = GetFramework();
  f.SetCurrentCountryChangedListener([observers](CountryId const & countryId)
  {
    for (TDrapeObserver observer in observers)
      if ([observer respondsToSelector:@selector(processViewportCountryEvent:)])
        [observer processViewportCountryEvent:countryId];
  });

  f.SetViewportListener([observers](ScreenBase const & screen)
  {
    for (TDrapeObserver observer in observers)
      if ([observer respondsToSelector:@selector(processViewportChangedEvent)])
        [observer processViewportChangedEvent];
  });
}

@end
