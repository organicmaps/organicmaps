
#import "TimeUtils.h"

@implementation NSDateFormatter (Seconds)

+ (NSString *)estimatedArrivalTimeWithSeconds:(NSNumber *)seconds
{
  static NSDateFormatter * dateFormatter;
  if (!dateFormatter)
  {
    dateFormatter = [NSDateFormatter new];
    dateFormatter.dateStyle = NSDateFormatterNoStyle;
    dateFormatter.timeStyle = NSDateFormatterShortStyle;
  }
  NSDate * date = [NSDate dateWithTimeIntervalSinceNow:[seconds floatValue]];
  NSString * string = [dateFormatter stringFromDate:date];
  return string;
}

@end