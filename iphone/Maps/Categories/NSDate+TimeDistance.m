#import "NSDate+TimeDistance.h"

@implementation NSDate (TimeDistance)

- (NSInteger)daysToNow {
  NSDateComponents *components = [[NSCalendar currentCalendar] components:NSCalendarUnitDay
                                                                 fromDate:self
                                                                   toDate:[NSDate date]
                                                                  options:NSCalendarWrapComponents];

  return components.day;
}

@end
