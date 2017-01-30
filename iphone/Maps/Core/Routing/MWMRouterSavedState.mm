#import "MWMRouterSavedState.h"
#import "MWMRoutePoint.h"
#import "MWMRouter.h"

#include "Framework.h"

static NSString * const kEndPointKey = @"endPoint";
static NSString * const kETAKey = @"eta";

@implementation MWMRouterSavedState

+ (instancetype)state
{
  static MWMRouterSavedState * state;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    state = [[super alloc] initState];
  });
  return state;
}

- (instancetype)initState
{
  self = [super init];
  if (self)
  {
    _forceStateChange = MWMRouterForceStateChange::None;
    NSDictionary * const stateDict =
        [NSDictionary dictionaryWithContentsOfURL:[[self class] stateFileURL]];
    if (stateDict)
    {
      m2::PointD point;
      NSUInteger size;
      NSGetSizeAndAlignment(@encode(m2::PointD), &size, NULL);
      NSData const * const endPointData = stateDict[kEndPointKey];
      NSDate * const eta = stateDict[kETAKey];
      if (endPointData && eta)
      {
        [endPointData getBytes:&point length:size];
        _restorePoint = routePoint(point, @"Destination");
        if ([eta compare:[NSDate date]] == NSOrderedDescending)
          _forceStateChange = MWMRouterForceStateChange::Rebuild;
      }
    }
  }
  return self;
}

+ (void)store
{
  Framework & f = GetFramework();
  if (!f.IsOnRoute())
    return;
  location::FollowingInfo routeInfo;
  f.GetRouteFollowingInfo(routeInfo);
  m2::PointD const endPoint = f.GetRouteEndPoint();
  NSMutableDictionary * const stateDict = [NSMutableDictionary dictionary];
  NSUInteger size;
  NSGetSizeAndAlignment(@encode(m2::PointD), &size, nullptr);
  stateDict[kEndPointKey] = [NSData dataWithBytes:&endPoint length:size];
  stateDict[kETAKey] = [NSDate dateWithTimeIntervalSinceNow:routeInfo.m_time];
  [stateDict writeToURL:[self stateFileURL] atomically:YES];
}

+ (void)remove
{
  [[NSFileManager defaultManager] removeItemAtURL:[self stateFileURL] error:nil];
}

+ (NSURL *)stateFileURL
{
  return [NSURL
      fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:@"route_info.ini"]];
}

+ (void)restore
{
  if (GetFramework().IsRoutingActive())
    return;
  if ([MWMRouterSavedState state].forceStateChange == MWMRouterForceStateChange::None)
    [self remove];
}

@end
