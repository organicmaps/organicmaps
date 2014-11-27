
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