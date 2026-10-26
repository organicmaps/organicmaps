#import "DurationFormatter.h"

#include "platform/duration.hpp"

@implementation DurationFormatter

+ (NSString *)routingDurationStringFromTimeInterval:(NSTimeInterval)timeInterval
{
  auto const duration = platform::Duration(timeInterval);
  return [NSString stringWithUTF8String:duration.GetRoutingString().c_str()];
}

@end
