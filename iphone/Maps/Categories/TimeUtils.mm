
#import "TimeUtils.h"

@implementation NSDateFormatter (Seconds)

+ (NSString *)estimatedArrivalTimeWithSeconds:(NSNumber *)seconds
{
  NSInteger const ti = [seconds integerValue];
  NSInteger const minutes = ti / 60;
  NSInteger const hours = minutes / 60;
  return [NSString stringWithFormat:@"%ld:%02ld", (long)hours, (long)(minutes % 60)];
}

+ (NSDate *)dateWithString:(NSString *)dateString
{
  static NSDateFormatter * dateFormatter;
  if (!dateFormatter)
  {
    dateFormatter = [NSDateFormatter new];
    dateFormatter.locale = [[NSLocale alloc] initWithLocaleIdentifier:@"en_US_POSIX"];
    dateFormatter.dateFormat = @"yyyy-MM-dd'T'HH:mm:ss";
  }
  NSDate * date = [dateFormatter dateFromString:dateString];
  return date;
}

@end