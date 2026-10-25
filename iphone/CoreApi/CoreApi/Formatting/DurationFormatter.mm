#import "DurationFormatter.h"

#include "platform/duration.hpp"

@implementation DurationFormatter

+ (NSString *)routingDurationStringFromTimeInterval:(NSTimeInterval)timeInterval
{
  auto const duration = platform::Duration(timeInterval);
  return [NSString stringWithCString:duration.GetRoutingString().c_str() encoding:NSUTF8StringEncoding];
}

@end
