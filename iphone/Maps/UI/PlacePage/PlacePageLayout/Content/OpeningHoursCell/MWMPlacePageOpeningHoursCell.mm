#import "MWMPlacePageOpeningHoursCell.h"
#import <CoreApi/MWMCommon.h>
#import <CoreApi/MWMOpeningHoursCommon.h>
#import "MWMPlacePageOpeningHoursDayView.h"
#import "SwiftBridge.h"

#include "editor/ui2oh.hpp"

using namespace editor;
using namespace osmoh;

using WeekDayView = MWMPlacePageOpeningHoursDayView *;

@interface MWMPlacePageOpeningHoursCell ()

@property(weak, nonatomic) IBOutlet WeekDayView currentDay;
@property(weak, nonatomic) IBOutlet UIView * middleSeparator;
@property(weak, nonatomic) IBOutlet UIView * weekDaysView;
@property(weak, nonatomic) IBOutlet UIImageView * expandImage;
@property(weak, nonatomic) IBOutlet UIButton * toggleButton;

@property(weak, nonatomic) IBOutlet UILabel * openTime;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * openTimeLeadingOffset;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * openTimeTrailingOffset;

@property(weak, nonatomic) IBOutlet NSLayoutConstraint * weekDaysViewHeight;
@property(nonatomic) CGFloat weekDaysViewEstimatedHeight;

@property(weak, nonatomic) id<MWMPlacePageOpeningHoursCellProtocol> delegate;

@property(nonatomic, readwrite) BOOL isClosed;
@property(nonatomic) BOOL haveExpandSchedule;

@end

NSString * stringFromTimeSpan(Timespan const & timeSpan)
{
  return [NSString stringWithFormat:@"%@ - %@", stringFromTime(timeSpan.GetStart()), stringFromTime(timeSpan.GetEnd())];
}

NSArray<NSString *> * arrayFromClosedTimes(TTimespans const & closedTimes)
{
  NSMutableArray<NSString *> * breaks = [NSMutableArray arrayWithCapacity:closedTimes.size()];
  for (auto & ct : closedTimes)
    [breaks addObject:stringFromTimeSpan(ct)];
  return [breaks copy];
}

WeekDayView getWeekDayView()
{
  return [NSBundle.mainBundle loadNibNamed:@"MWMPlacePageOpeningHoursWeekDayView" owner:nil options:nil].firstObject;
}

@implementation MWMPlacePageOpeningHoursCell
{
  ui::TimeTableSet timeTableSet;
}

- (void)configWithDelegate:(id<MWMPlacePageOpeningHoursCellProtocol>)delegate info:(NSString *)info
{
  self.delegate = delegate;
  WeekDayView cd = self.currentDay;
  cd.currentDay = YES;

  self.toggleButton.hidden = !delegate.forcedButton;
  self.expandImage.hidden = !delegate.forcedButton;
  self.expandImage.image = [UIImage imageNamed:@"ic_arrow_gray_right"];
  self.expandImage.styleName = @"MWMGray";
  if (isInterfaceRightToLeft())
    self.expandImage.transform = CGAffineTransformMakeScale(-1, 1);
  NSAssert(info, @"Schedule can not be empty");
  osmoh::OpeningHours oh(info.UTF8String);
  if (MakeTimeTableSet(oh, timeTableSet))
  {
    cd.isCompatibility = NO;
    if (delegate.isEditor)
      self.isClosed = NO;
    else
      self.isClosed = oh.IsClosed(time(nullptr));
    [self processSchedule];
  }
  else
  {
    cd.isCompatibility = YES;
    [cd setCompatibilityText:info isPlaceholder:delegate.isPlaceholder];
  }
  BOOL const isHidden = !self.isExpanded;
  self.middleSeparator.hidden = isHidden;
  self.weekDaysView.hidden = isHidden;
  [cd invalidate];
}

- (void)processSchedule
{
  NSCalendar * cal = NSCalendar.currentCalendar;
  cal.locale = NSLocale.currentLocale;
  Weekday currentDay = static_cast<Weekday>([cal components:NSCalendarUnitWeekday fromDate:[NSDate date]].weekday);
  BOOL haveCurrentDay = NO;
  size_t timeTablesCount = timeTableSet.Size();
  self.haveExpandSchedule = (timeTablesCount > 1 || !timeTableSet.GetUnhandledDays().empty());
  self.weekDaysViewEstimatedHeight = 0.0;
  [self.weekDaysView.subviews makeObjectsPerformSelector:@selector(removeFromSuperview)];
  for (size_t idx = 0; idx < timeTablesCount; ++idx)
  {
    auto tt = timeTableSet.Get(idx);
    ui::OpeningDays const & workingDays = tt.GetOpeningDays();
    if (workingDays.find(currentDay) != workingDays.end())
    {
      haveCurrentDay = YES;
      [self addCurrentDay:tt];
    }
    if (self.isExpanded)
      [self addWeekDays:tt];
  }
  if (!haveCurrentDay)
    [self addEmptyCurrentDay];
  id<MWMPlacePageOpeningHoursCellProtocol> delegate = self.delegate;
  if (self.haveExpandSchedule)
  {
    self.toggleButton.hidden = NO;
    self.expandImage.hidden = NO;
    if (delegate.forcedButton)
      self.expandImage.image = [UIImage imageNamed:@"ic_arrow_gray_right"];
    else if (self.isExpanded)
      self.expandImage.image = [UIImage imageNamed:@"ic_arrow_gray_up"];
    else
      self.expandImage.image = [UIImage imageNamed:@"ic_arrow_gray_down"];

    self.expandImage.styleName = @"MWMGray";
    if (isInterfaceRightToLeft())
      self.expandImage.transform = CGAffineTransformMakeScale(-1, 1);

    if (self.isExpanded)
      [self addClosedDays];
  }
  self.openTimeTrailingOffset.priority =
      delegate.forcedButton ? UILayoutPriorityDefaultHigh : UILayoutPriorityDefaultLow;
  self.weekDaysViewHeight.constant = ceil(self.weekDaysViewEstimatedHeight);
  [self alignTimeOffsets];
}

- (void)addCurrentDay:(ui::TimeTableSet::Proxy)timeTable
{
  WeekDayView cd = self.currentDay;
  NSString * label;
  NSString * openTime;
  NSArray<NSString *> * breaks;

  BOOL const everyDay = isEveryDay(timeTable);
  if (timeTable.IsTwentyFourHours())
  {
    label = everyDay ? L(@"twentyfour_seven") : L(@"editor_time_allday");
    openTime = @"";
    breaks = @[];
  }
  else
  {
    self.haveExpandSchedule |= !everyDay;
    label = everyDay ? L(@"daily") : L(@"today");
    openTime = stringFromTimeSpan(timeTable.GetOpeningTime());
    breaks = arrayFromClosedTimes(timeTable.GetExcludeTime());
  }

  [cd setLabelText:label isRed:NO];
  [cd setOpenTimeText:openTime];
  [cd setBreaks:breaks];
  [cd setClosed:self.isClosed];
}

- (void)addEmptyCurrentDay
{
  WeekDayView cd = self.currentDay;
  [cd setLabelText:L(@"day_off_today") isRed:YES];
  [cd setOpenTimeText:@""];
  [cd setBreaks:@[]];
  [cd setClosed:NO];
}

- (void)addWeekDays:(ui::TimeTableSet::Proxy)timeTable
{
  WeekDayView wd = getWeekDayView();
  wd.currentDay = NO;
  wd.frame = {{0, self.weekDaysViewEstimatedHeight}, {self.weekDaysView.width, 0}};
  [wd setLabelText:stringFromOpeningDays(timeTable.GetOpeningDays()) isRed:NO];
  if (timeTable.IsTwentyFourHours())
  {
    BOOL const everyDay = isEveryDay(timeTable);
    [wd setOpenTimeText:everyDay ? L(@"twentyfour_seven") : L(@"editor_time_allday")];
    [wd setBreaks:@[]];
  }
  else
  {
    [wd setOpenTimeText:stringFromTimeSpan(timeTable.GetOpeningTime())];
    [wd setBreaks:arrayFromClosedTimes(timeTable.GetExcludeTime())];
  }
  [wd invalidate];
  [self.weekDaysView addSubview:wd];
  self.weekDaysViewEstimatedHeight += wd.viewHeight;
}

- (void)addClosedDays
{
  editor::ui::OpeningDays closedDays = timeTableSet.GetUnhandledDays();
  if (closedDays.empty())
    return;
  WeekDayView wd = getWeekDayView();
  wd.currentDay = NO;
  wd.frame = {{0, self.weekDaysViewEstimatedHeight}, {self.weekDaysView.width, 0}};
  [wd setLabelText:stringFromOpeningDays(closedDays) isRed:NO];
  [wd setOpenTimeText:L(@"day_off")];
  [wd setBreaks:@[]];
  [wd invalidate];
  [self.weekDaysView addSubview:wd];
  self.weekDaysViewEstimatedHeight += wd.viewHeight;
}

- (void)alignTimeOffsets
{
  CGFloat offset = self.openTime.minX;
  for (WeekDayView wd in self.weekDaysView.subviews)
    offset = MAX(offset, wd.openTimeLeadingOffset);

  for (WeekDayView wd in self.weekDaysView.subviews)
    wd.openTimeLeadingOffset = offset;
}

- (CGFloat)cellHeight
{
  CGFloat height = self.currentDay.viewHeight;
  if (self.isExpanded)
  {
    CGFloat const bottomOffset = 4.0;
    height += bottomOffset;
    if (!self.currentDay.isCompatibility)
      height += self.weekDaysViewHeight.constant;
  }
  return ceil(height);
}

#pragma mark - Actions

- (IBAction)toggleButtonTap
{
  id<MWMPlacePageOpeningHoursCellProtocol> delegate = self.delegate;
  [delegate setOpeningHoursCellExpanded:!delegate.openingHoursCellExpanded];

  // Workaround for slow devices.
  // Major QA can tap multiple times before first segue call is performed.
  // This leads to multiple identical controllers to be pushed.
  self.toggleButton.enabled = NO;
  dispatch_async(dispatch_get_main_queue(), ^{ self.toggleButton.enabled = YES; });
}

#pragma mark - Properties

- (BOOL)isExpanded
{
  if (self.currentDay.isCompatibility || !self.haveExpandSchedule)
    return NO;
  return self.delegate.openingHoursCellExpanded;
}

@end
