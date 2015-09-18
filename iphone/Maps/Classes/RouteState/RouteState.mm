#import "RouteState.h"

#include "Framework.h"

static NSString * const kEndPointKey = @"endPoint";
static NSString * const kETAKey = @"eta";

@interface RouteState()

@property (nonatomic) NSDate * eta;

@end

@implementation RouteState

+ (instancetype)savedState
{
  RouteState * const state = [[super alloc] init];
  if (state)
  {
    NSDictionary * const stateDict = [NSDictionary dictionaryWithContentsOfURL:[RouteState stateFileURL]];
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
        state.endPoint = point;
        state.eta = eta;
      }
    }
  }
  return state;
}

+ (void)save
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
  [stateDict writeToURL:[RouteState stateFileURL] atomically:YES];
}

+ (void)remove
{
  [[NSFileManager defaultManager] removeItemAtURL:[RouteState stateFileURL] error:nil];
}

+ (NSURL *)stateFileURL
{
  return [NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:@"route_info.ini"]];
}

#pragma mark - Properties

- (BOOL)hasActualRoute
{
  return [self.eta compare:[NSDate date]] == NSOrderedDescending;
}

@end
