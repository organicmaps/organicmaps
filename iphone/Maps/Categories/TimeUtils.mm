#import "TimeUtils.h"
#import "Common.h"

@implementation NSDateFormatter (Seconds)

+ (NSString *)estimatedArrivalTimeWithSeconds:(NSTimeInterval)seconds
{
  NSDateComponentsFormatter * formatter = [[NSDateComponentsFormatter alloc] init];
  formatter.allowedUnits = NSCalendarUnitMinute | NSCalendarUnitHour | NSCalendarUnitDay;
  formatter.maximumUnitCount = 2;
  formatter.unitsStyle = NSDateComponentsFormatterUnitsStyleAbbreviated;
  formatter.zeroFormattingBehavior = NSDateComponentsFormatterZeroFormattingBehaviorDropAll;
  return [formatter stringFromTimeInterval:seconds];
}

@end
