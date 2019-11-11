#import "OpeningHours.h"

#import "MWMOpeningHours.h"

@interface WorkingDay ()

@property(nonatomic, copy) NSString *workingDays;
@property(nonatomic, copy) NSString *workingTimes;
@property(nonatomic, copy) NSString *breaks;

@end

@implementation WorkingDay

@end

@interface OpeningHours ()

@property(nonatomic, strong) NSArray<WorkingDay *> *days;

@end

@implementation OpeningHours

- (instancetype)initWithRawString:(NSString *)rawString localization:(id<IOpeningHoursLocalization>)localization {
  self = [super init];
  if (self) {
    auto days = osmoh::processRawString(rawString, localization);
    NSMutableArray *array = [NSMutableArray arrayWithCapacity:days.size()];
    for (auto day : days) {
      WorkingDay *wd = [[WorkingDay alloc] init];
      wd.workingDays = day.m_workingDays;
      wd.workingTimes = day.m_workingTimes.length > 0 ? day.m_workingTimes : localization.closedString;
      wd.breaks = day.m_breaks;
      [array addObject:wd];
    }
    _days = [array copy];
  }
  return self;
}

@end
