#import "MWMWatchEventInfo.h"

static NSString * const kRequestKey = @"request";

@implementation NSDictionary (MWMWatchEventInfo)

- (MWMWatchEventInfoRequest)watchEventInfoRequest
{
  NSNumber * requestValue = self[kRequestKey];
  return (MWMWatchEventInfoRequest)requestValue.unsignedIntegerValue;
}

@end

@implementation NSMutableDictionary (MWMWatchEventInfo)

- (void)setWatchEventInfoRequest:(MWMWatchEventInfoRequest)watchEventInfoRequest
{
  NSNumber * requestValue = @(watchEventInfoRequest);
  self[kRequestKey] = requestValue;
}

@end
