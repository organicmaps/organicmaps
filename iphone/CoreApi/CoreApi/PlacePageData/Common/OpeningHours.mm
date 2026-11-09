#import "OpeningHours.h"

#import "MWMOpeningHours.h"
#include "opening_hours/opening_hours.hpp"

@interface WorkingDay ()

@property(nonatomic, copy) NSString * workingDays;
@property(nonatomic, copy) NSString * workingTimes;
@property(nonatomic) BOOL isOpen;

@end

@implementation WorkingDay

@end

@interface OpeningHours ()

@property(nonatomic, strong) NSArray<WorkingDay *> * days;

@end

@implementation OpeningHours

- (instancetype)initWithRawString:(NSString *)rawString localization:(id<IOpeningHoursLocalization>)localization
{
  NSInteger const today = [NSCalendar.currentCalendar component:NSCalendarUnitWeekday fromDate:NSDate.date];
  return [self initWithRawString:rawString localization:localization today:today];
}

- (instancetype)initWithRawString:(NSString *)rawString
                     localization:(id<IOpeningHoursLocalization>)localization
                            today:(NSInteger)today
{
  auto const days = osmoh::processRawString(rawString, localization, today);
  if (!days)
    return nil;

  self = [super init];
  if (self)
  {
    NSMutableArray * array = [NSMutableArray arrayWithCapacity:days->size()];
    for (auto const & day : *days)
    {
      WorkingDay * wd = [[WorkingDay alloc] init];
      wd.isOpen = day.m_isOpen;
      wd.workingDays = day.m_workingDays;
      wd.workingTimes = day.m_isOpen ? day.m_workingTimes : localization.closedString;
      [array addObject:wd];
    }
    _days = [array copy];
  }
  return self;
}

@end
