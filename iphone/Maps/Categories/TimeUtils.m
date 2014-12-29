
#import "TimeUtils.h"

@implementation NSDateFormatter (Seconds)

+ (NSString *)estimatedArrivalTimeWithSeconds:(NSNumber *)seconds
{
  NSInteger ti = [seconds integerValue];
  NSInteger minutes = (ti / 60) % 60;
  NSInteger hours = (ti / 3600);
  return [NSString stringWithFormat:@"%ld:%02ld", (long)hours, (long)minutes];
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