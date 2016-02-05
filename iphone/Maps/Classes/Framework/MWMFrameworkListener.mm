#import "MapsAppDelegate.h"
#import "MWMFrameworkListener.h"

#include "Framework.h"

#include "std/mutex.hpp"

namespace
{
using TObserver = id<MWMFrameworkObserver>;
using TRouteBuildingObserver = id<MWMFrameworkRouteBuilderObserver>;
using TMyPositionObserver = id<MWMFrameworkMyPositionObserver>;
using TUsermarkObserver = id<MWMFrameworkUserMarkObserver>;
using TStorageObserver = id<MWMFrameworkStorageObserver>;

using TObservers = NSHashTable<__kindof TObserver>;

Protocol * pRouteBuildingObserver = @protocol(MWMFrameworkRouteBuilderObserver);
Protocol * pMyPositionObserver = @protocol(MWMFrameworkMyPositionObserver);
Protocol * pUserMarkObserver = @protocol(MWMFrameworkUserMarkObserver);
Protocol * pStorageObserver = @protocol(MWMFrameworkStorageObserver);

using TLoopBlock = void (^)(__kindof TObserver observer);

void loopWrappers(TObservers * observers, TLoopBlock block)
{
  dispatch_async(dispatch_get_main_queue(), ^
  {
    for (TObserver observer in observers)
    {
      if (observer)
        block(observer);
    }
  });
}
} // namespace

@interface MWMFrameworkListener ()

@property (nonatomic) TObservers * routeBuildingObservers;
@property (nonatomic) TObservers * myPositionObservers;
@property (nonatomic) TObservers * userMarkObservers;
@property (nonatomic) TObservers * storageObservers;

@property (nonatomic, readwrite) location::EMyPositionMode myPositionMode;

@end

@implementation MWMFrameworkListener
{
  unique_ptr<UserMarkCopy> m_userMark;
  mutex m_userMarkMutex;
}

+ (MWMFrameworkListener *)listener
{
  return MapsAppDelegate.theApp.frameworkListener;
}

- (instancetype)init
{
  self = [super init];
  if (self)
  {
    _routeBuildingObservers = [TObservers weakObjectsHashTable];
    _myPositionObservers = [TObservers weakObjectsHashTable];
    _userMarkObservers = [TObservers weakObjectsHashTable];
    _storageObservers = [TObservers weakObjectsHashTable];

    [self registerRouteBuilderListener];
    [self registerMyPositionListener];
    [self registerUserMarkObserver];
    [self registerStorageObserver];
  }
  return self;
}

- (void)addObserver:(TObserver)observer
{
  dispatch_async(dispatch_get_main_queue(), ^
  {
    if ([observer conformsToProtocol:pRouteBuildingObserver])
      [self.routeBuildingObservers addObject:observer];
    if ([observer conformsToProtocol:pMyPositionObserver])
      [self.myPositionObservers addObject:observer];
    if ([observer conformsToProtocol:pUserMarkObserver])
      [self.userMarkObservers addObject:observer];
    if ([observer conformsToProtocol:pStorageObserver])
      [self.storageObservers addObject:observer];
  });
}

#pragma mark - MWMFrameworkRouteBuildingObserver

- (void)registerRouteBuilderListener
{
  using namespace routing;
  using namespace storage;
  TObservers * observers = self.routeBuildingObservers;
  auto & f = GetFramework();
  f.SetRouteBuildingListener([observers](IRouter::ResultCode code, TCountriesVec const & absentCountries, TCountriesVec const & absentRoutes)
  {
    loopWrappers(observers, [code, absentCountries, absentRoutes](TRouteBuildingObserver observer)
    {
      [observer processRouteBuilderEvent:code countries:absentCountries routes:absentRoutes];
    });
  });
  f.SetRouteProgressListener([observers](float progress)
  {
    loopWrappers(observers, [progress](TRouteBuildingObserver observer)
    {
      if ([observer respondsToSelector:@selector(processRouteBuilderProgress:)])
        [observer processRouteBuilderProgress:progress];
    });
  });
}

#pragma mark - MWMFrameworkMyPositionObserver

- (void)registerMyPositionListener
{
  TObservers * observers = self.myPositionObservers;
  auto & f = GetFramework();
  f.SetMyPositionModeListener([self, observers](location::EMyPositionMode mode)
  {
    self.myPositionMode = mode;
    loopWrappers(observers, [mode](TMyPositionObserver observer)
    {
      [observer processMyPositionStateModeChange:mode];
    });
  });
}

#pragma mark - MWMFrameworkUserMarkObserver

- (void)registerUserMarkObserver
{
  TObservers * observers = self.userMarkObservers;
  auto & f = GetFramework();
  f.SetUserMarkActivationListener([self, observers](unique_ptr<UserMarkCopy> mark)
  {
    lock_guard<mutex> lock(m_userMarkMutex);
    m_userMark = move(mark);
    loopWrappers(observers, [self](TUsermarkObserver observer)
    {
      lock_guard<mutex> lock(self->m_userMarkMutex);
      if (self->m_userMark != nullptr)
        [observer processUserMarkActivation:self->m_userMark->GetUserMark()];
      else
        [observer processUserMarkActivation:nullptr];
    });
  });
}

#pragma mark - MWMFrameworkStorageObserver

- (void)registerStorageObserver
{
  TObservers * observers = self.storageObservers;
  auto & s = GetFramework().Storage();
  s.Subscribe([observers](storage::TCountryId const & countryId)
  {
    loopWrappers(observers, [countryId](TStorageObserver observer)
    {
      [observer processCountryEvent:countryId];
    });
  },
  [observers](storage::TCountryId const & countryId, storage::TLocalAndRemoteSize const & progress)
  {
    loopWrappers(observers, [countryId, progress](TStorageObserver observer)
    {
      if ([observer respondsToSelector:@selector(processCountry:progress:)])
        [observer processCountry:countryId progress:progress];
    });
  });
}

#pragma mark - Properties

- (UserMark const *)userMark
{
  return m_userMark ? m_userMark->GetUserMark() : nullptr;
}

- (void)setUserMark:(const UserMark *)userMark
{
  if (userMark)
    m_userMark.reset(new UserMarkCopy(userMark, false));
  else
    m_userMark = nullptr;
}

@end
