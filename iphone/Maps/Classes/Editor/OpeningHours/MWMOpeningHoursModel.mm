#import "MWMOpeningHoursModel.h"
#import "MWMOpeningHoursSection.h"

#include "editor/opening_hours_ui.hpp"

extern UITableViewRowAnimation const kMWMOpeningHoursEditorRowAnimation = UITableViewRowAnimationFade;

@interface MWMOpeningHoursModel () <MWMOpeningHoursSectionProtocol>

@property (weak, nonatomic, readwrite) UITableView * tableView;

@property (nonatomic) NSMutableArray<MWMOpeningHoursSection *> * sections;

@end

using namespace editor::ui;
using namespace osmoh;

@implementation MWMOpeningHoursModel
{
  TimeTableSet timeTableSet;
}

- (instancetype _Nullable)initWithDelegate:(id<MWMOpeningHoursModelProtocol> _Nonnull)delegate
{
  self = [super init];
  if (self)
  {
    _tableView = delegate.tableView;
    _sections = [NSMutableArray arrayWithCapacity:timeTableSet.Size()];
    while (self.sections.count < timeTableSet.Size())
      [self addSection];
  }
  return self;
}

- (void)addSection
{
  [self.sections addObject:[[MWMOpeningHoursSection alloc] initWithDelegate:self]];
  [self refreshSectionsIndexes];
  NSIndexSet * set = [[NSIndexSet alloc] initWithIndex:self.count - 1];
  [self.tableView reloadSections:set withRowAnimation:kMWMOpeningHoursEditorRowAnimation];
}

- (void)refreshSectionsIndexes
{
  [self.sections enumerateObjectsUsingBlock:^(MWMOpeningHoursSection * _Nonnull section,
                                              NSUInteger idx, BOOL * _Nonnull stop)
  {
    [section refreshIndex:idx];
  }];
}

- (void)addSchedule
{
  NSAssert(self.canAddSection, @"Can not add schedule");
  timeTableSet.Append(timeTableSet.GetComplementTimeTable());
  [self addSection];
  NSAssert(timeTableSet.Size() == self.sections.count, @"Inconsistent state");
  [self.sections[self.sections.count - 1] scrollIntoView];
}

- (void)deleteSchedule:(NSUInteger)index
{
  NSAssert(index < self.count, @"Invalid section index");
  BOOL const needRealDelete = self.canAddSection;
  timeTableSet.Remove(index);
  [self.sections removeObjectAtIndex:index];
  [self refreshSectionsIndexes];
  if (needRealDelete)
  {
    NSIndexSet * set = [[NSIndexSet alloc] initWithIndex:index];
    [self.tableView deleteSections:set withRowAnimation:kMWMOpeningHoursEditorRowAnimation];
  }
  else
  {
    NSIndexSet * set = [[NSIndexSet alloc] initWithIndexesInRange:{index, self.count - index + 1}];
    [self.tableView reloadSections:set withRowAnimation:kMWMOpeningHoursEditorRowAnimation];
  }
}

- (void)updateActiveSection:(NSUInteger)index
{
  for (MWMOpeningHoursSection * section in self.sections)
  {
    if (section.index != index)
      section.selectedRow = nil;
  }
}

- (TTimeTableProxy)getTimeTableProxy:(NSUInteger)index
{
  NSAssert(index < self.count, @"Invalid section index");
  return timeTableSet.Get(index);
}

- (MWMOpeningHoursEditorCells)cellKeyForIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  NSUInteger const section = indexPath.section;
  NSAssert(section < self.count, @"Invalid section index");
  return [self.sections[section] cellKeyForRow:indexPath.row];
}

- (CGFloat)heightForIndexPath:(NSIndexPath * _Nonnull)indexPath withWidth:(CGFloat)width
{
  NSUInteger const section = indexPath.section;
  NSAssert(section < self.count, @"Invalid section index");
  return [self.sections[section] heightForRow:indexPath.row withWidth:width];
}

- (void)fillCell:(MWMOpeningHoursTableViewCell * _Nonnull)cell atIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  NSUInteger const section = indexPath.section;
  NSAssert(section < self.count, @"Invalid section index");
  cell.indexPathAtInit = indexPath;
  cell.section = self.sections[section];
}

- (NSUInteger)numberOfRowsInSection:(NSUInteger)section
{
  NSAssert(section < self.count, @"Invalid section index");
  return self.sections[section].numberOfRows;
}

#pragma mark - Properties

- (NSUInteger)count
{
  NSAssert(timeTableSet.Size() == self.sections.count, @"Inconsistent state");
  return self.sections.count;
}

- (BOOL)canAddSection
{
  return !timeTableSet.GetUnhandledDays().empty();
}

- (NSArray<NSString *> *)unhandledDays
{
  auto const unhandledDays = timeTableSet.GetUnhandledDays();
  NSCalendar * cal = [NSCalendar currentCalendar];
  cal.locale = [NSLocale currentLocale];
  NSUInteger const firstWeekday = [cal firstWeekday] - 1;
  Weekday (^day2Weekday)(NSUInteger) = ^Weekday(NSUInteger day)
  {
    NSUInteger idx = day + 1;
    if (idx > static_cast<NSUInteger>(Weekday::Saturday))
      idx -= static_cast<NSUInteger>(Weekday::Saturday);
    return static_cast<Weekday>(idx);
  };

  NSArray<NSString *> * weekdaySymbols = cal.shortStandaloneWeekdaySymbols;
  NSMutableArray<NSString *> * dayNames = [NSMutableArray arrayWithCapacity:unhandledDays.size()];
  NSUInteger const weekDaysCount = 7;
  for (NSUInteger i = 0, day = firstWeekday; i < weekDaysCount; ++i, ++day)
  {
    Weekday const wd = day2Weekday(day);
    if (unhandledDays.find(wd) != unhandledDays.end())
      [dayNames addObject:weekdaySymbols[static_cast<NSInteger>(wd) - 1]];
  }
  return dayNames;
}

@end
