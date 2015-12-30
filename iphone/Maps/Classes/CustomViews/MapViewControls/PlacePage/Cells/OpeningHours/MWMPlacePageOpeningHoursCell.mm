#import "MWMOpeningHoursCommon.h"
#import "MWMPlacePageOpeningHoursCell.h"
#import "MWMPlacePageOpeningHoursDayView.h"
#import "Statistics.h"

#include "3party/opening_hours/opening_hours.hpp"
#include "editor/opening_hours_ui.hpp"
#include "editor/ui2oh.hpp"

using namespace editor;
using namespace osmoh;

using WeekDayView = MWMPlacePageOpeningHoursDayView *;

@interface MWMPlacePageOpeningHoursCell ()

@property (weak, nonatomic) IBOutlet WeekDayView currentDay;
@property (weak, nonatomic) IBOutlet UIView * middleSeparator;
@property (weak, nonatomic) IBOutlet UIView * weekDaysView;
@property (weak, nonatomic) IBOutlet UIButton * editButton;
@property (weak, nonatomic) IBOutlet UIImageView * expandImage;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * weekDaysViewHeight;
@property (nonatomic) CGFloat weekDaysViewEstimatedHeight;

@property (weak, nonatomic) id<MWMPlacePageOpeningHoursCellProtocol> delegate;

@property (nonatomic) BOOL isClosed;

@end

NSString * stringFromTimeSpan(Timespan const & timeSpan)
{
  return [NSString stringWithFormat:@"%@-%@", stringFromTime(timeSpan.GetStart()), stringFromTime(timeSpan.GetEnd())];
}

NSArray<NSString *> * arrayFromClosedTimes(TTimespans const & closedTimes)
{
  NSMutableArray<NSString *> * breaks = [NSMutableArray arrayWithCapacity:closedTimes.size()];
  for(auto & ct : closedTimes)
  {
    [breaks addObject:stringFromTimeSpan(ct)];
  }
  return [breaks copy];
}

WeekDayView getWeekDayView()
{
  return [[[NSBundle mainBundle] loadNibNamed:@"MWMPlacePageOpeningHoursWeekDayView"
                                        owner:nil
                                      options:nil] firstObject];
}

@implementation MWMPlacePageOpeningHoursCell
{
  ui::TimeTableSet timeTableSet;
}

- (void)configWithInfo:(NSString *)info delegate:(id<MWMPlacePageOpeningHoursCellProtocol>)delegate
{
  self.delegate = delegate;
  WeekDayView cd = self.currentDay;
  cd.currentDay = YES;

  if (info)
  {
    osmoh::OpeningHours oh(info.UTF8String);
    if (MakeTimeTableSet(oh, timeTableSet))
    {
      cd.mode = MWMPlacePageOpeningHoursDayViewModeRegular;
      self.isClosed = oh.IsClosed(time(nullptr));
      [self processSchedule];

      BOOL const isExpanded = delegate.openingHoursCellExpanded;
      self.middleSeparator.hidden = !isExpanded;
      self.weekDaysView.hidden = !isExpanded;
      self.editButton.hidden = !isExpanded;
      self.expandImage.image = [UIImage imageNamed:isExpanded ? @"ic_arrow_gray_up" : @"ic_arrow_gray_down"];
      [cd invalidate];
      return;
    }
    cd.mode = MWMPlacePageOpeningHoursDayViewModeCompatibility;
    [cd setCompatibilityText:info];
  }
  else
  {
    cd.mode = MWMPlacePageOpeningHoursDayViewModeEmpty;
  }

  self.middleSeparator.hidden = YES;
  self.weekDaysView.hidden = YES;
  self.editButton.hidden = YES;
  self.expandImage.hidden = YES;
  [cd invalidate];
}

- (void)processSchedule
{
  NSCalendar * cal = [NSCalendar currentCalendar];
  cal.locale = [NSLocale currentLocale];
  Weekday currentDay = static_cast<Weekday>([cal component:NSCalendarUnitWeekday fromDate:[NSDate date]]);
  BOOL haveCurrentDay = NO;
  size_t timeTablesCount = timeTableSet.Size();
  BOOL const haveExpandSchedule = (timeTablesCount > 1);
  BOOL const isExpanded = self.delegate.openingHoursCellExpanded;
  self.weekDaysViewEstimatedHeight = 0.0;
  [self.weekDaysView.subviews makeObjectsPerformSelector:@selector(removeFromSuperview)];
  [self.currentDay setCanExpand:YES];
  for (size_t idx = 0; idx < timeTablesCount; ++idx)
  {
    ui::TTimeTableProxy tt = timeTableSet.Get(idx);
    ui::TOpeningDays const & workingDays = tt.GetOpeningDays();
    if (workingDays.find(currentDay) != workingDays.end())
    {
      haveCurrentDay = YES;
      [self addCurrentDay:tt];
    }
    if (haveExpandSchedule && isExpanded)
      [self addWeekDays:tt];
  }
  if (!haveCurrentDay)
    [self addEmptyCurrentDay];
  if (haveExpandSchedule && isExpanded)
    [self addClosedDays];
  self.weekDaysViewHeight.constant = ceil(self.weekDaysViewEstimatedHeight);
}

- (void)addCurrentDay:(ui::TTimeTableProxy)timeTable
{
  WeekDayView cd = self.currentDay;
  NSString * label;
  NSString * openTime;
  NSArray<NSString *> * breaks;

  if (timeTable.IsTwentyFourHours())
  {
    label = L(@"24/7");
    openTime = @"";
    breaks = @[];
  }
  else
  {
    BOOL const everyDay = (timeTable.GetOpeningDays().size() == 7);
    label = everyDay ? L(@"every_day") : L(@"today");
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
  [cd setLabelText:L(@"closed_today") isRed:YES];
  [cd setOpenTimeText:@""];
  [cd setBreaks:@[]];
  [cd setClosed:NO];
}

- (void)addWeekDays:(ui::TTimeTableProxy)timeTable
{
  WeekDayView wd = getWeekDayView();
  wd.currentDay = NO;
  wd.frame = {{0, self.weekDaysViewEstimatedHeight}, {self.weekDaysView.width, 0}};
  [wd setLabelText:stringFromOpeningDays(timeTable.GetOpeningDays()) isRed:NO];
  [wd setOpenTimeText:stringFromTimeSpan(timeTable.GetOpeningTime())];
  [wd setBreaks:arrayFromClosedTimes(timeTable.GetExcludeTime())];
  [wd invalidate];
  [self.weekDaysView addSubview:wd];
  self.weekDaysViewEstimatedHeight += wd.viewHeight;
}

- (void)addClosedDays
{
  editor::ui::TOpeningDays closedDays = timeTableSet.GetUnhandledDays();
  if (closedDays.empty())
    return;
  WeekDayView wd = getWeekDayView();
  wd.currentDay = NO;
  wd.frame = {{0, self.weekDaysViewEstimatedHeight}, {self.weekDaysView.width, 0}};
  [wd setLabelText:stringFromOpeningDays(closedDays) isRed:NO];
  [wd setOpenTimeText:L(@"closed_this_day")];
  [wd setBreaks:@[]];
  [wd invalidate];
  [self.weekDaysView addSubview:wd];
  self.weekDaysViewEstimatedHeight += wd.viewHeight;
}

- (CGFloat)cellHeight
{
  CGFloat height = self.currentDay.viewHeight;
  BOOL const isRegular = (self.currentDay.mode == MWMPlacePageOpeningHoursDayViewModeRegular);
  if (isRegular && self.delegate.openingHoursCellExpanded)
    height += self.weekDaysViewHeight.constant + self.editButton.height;
  return ceil(height);
}

#pragma mark - Actions

- (IBAction)toggleButtonTap
{
  switch (self.currentDay.mode) {
    case MWMPlacePageOpeningHoursDayViewModeRegular:
      [self.delegate setOpeningHoursCellExpanded:!self.delegate.openingHoursCellExpanded forCell:self];
      break;
    case MWMPlacePageOpeningHoursDayViewModeCompatibility:
      break;
    case MWMPlacePageOpeningHoursDayViewModeEmpty:
      [self editButtonTap];
      break;
  }
}

- (IBAction)editButtonTap
{
  [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatEditTime)];
  [self.delegate editPlaceTime];
}

@end
