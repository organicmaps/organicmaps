#import "TimeUtils.h"
#import "Common.h"

@implementation NSDateFormatter (Seconds)

+ (NSString *)estimatedArrivalTimeWithSeconds:(NSTimeInterval)seconds
{
  if (isIOS7)
  {
    NSInteger const minutes = seconds / 60;
    NSInteger const hours = minutes / 60;
    return [NSString stringWithFormat:@"%ld:%02ld", (long)hours, (long)(minutes % 60)];
  }
  else
  {
    NSDateComponentsFormatter * formatter = [[NSDateComponentsFormatter alloc] init];
    formatter.allowedUnits = NSCalendarUnitMinute | NSCalendarUnitHour | NSCalendarUnitDay;
    formatter.maximumUnitCount = 2;
    formatter.unitsStyle = NSDateComponentsFormatterUnitsStyleAbbreviated;
    formatter.zeroFormattingBehavior = NSDateComponentsFormatterZeroFormattingBehaviorDropAll;
    return [formatter stringFromTimeInterval:seconds];
  }
}

@end