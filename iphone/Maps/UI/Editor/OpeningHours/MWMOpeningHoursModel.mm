#import "MWMOpeningHoursModel.h"
#import <CoreApi/MWMOpeningHoursCommon.h>

#include "editor/ui2oh.hpp"

extern UITableViewRowAnimation const kMWMOpeningHoursEditorRowAnimation = UITableViewRowAnimationFade;

@interface MWMOpeningHoursModel () <MWMOpeningHoursSectionProtocol>

@property(weak, nonatomic) id<MWMOpeningHoursModelProtocol> delegate;

@property(nonatomic) NSMutableArray<MWMOpeningHoursSection *> * sections;

@end

using namespace editor;
using namespace osmoh;

@implementation MWMOpeningHoursModel
{
  ui::TimeTableSet timeTableSet;
}

- (instancetype _Nullable)initWithDelegate:(id<MWMOpeningHoursModelProtocol> _Nonnull)delegate
{
  self = [super init];
  if (self)
    _delegate = delegate;
  return self;
}

- (void)addSection
{
  [self.sections addObject:[[MWMOpeningHoursSection alloc] initWithDelegate:self]];
  [self refreshSectionsIndexes];
}

- (void)refreshSectionsIndexes
{
  [self.sections enumerateObjectsUsingBlock:^(MWMOpeningHoursSection * _Nonnull section, NSUInteger idx,
                                              BOOL * _Nonnull stop) { [section refreshIndex:idx]; }];
}

- (void)addSchedule
{
  NSAssert(self.canAddSection, @"Can not add schedule");
  timeTableSet.Append(timeTableSet.GetComplementTimeTable());
  [self addSection];
  [self.tableView reloadSections:[[NSIndexSet alloc] initWithIndex:self.sections.count - 1]
                withRowAnimation:kMWMOpeningHoursEditorRowAnimation];
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
  UITableView * tableView = self.tableView;
  if (needRealDelete)
  {
    [tableView deleteSections:[[NSIndexSet alloc] initWithIndex:index]
             withRowAnimation:kMWMOpeningHoursEditorRowAnimation];
    [tableView reloadSections:[[NSIndexSet alloc] initWithIndex:self.count]
             withRowAnimation:kMWMOpeningHoursEditorRowAnimation];
  }
  else
  {
    NSRange reloadRange = {index, self.count - index + 1};
    [tableView reloadSections:[[NSIndexSet alloc] initWithIndexesInRange:reloadRange]
             withRowAnimation:kMWMOpeningHoursEditorRowAnimation];
  }
}

- (void)updateActiveSection:(NSUInteger)index
{
  for (MWMOpeningHoursSection * section in self.sections)
    if (section.index != index)
      section.selectedRow = nil;
}

- (ui::TimeTableSet::Proxy)timeTableProxy:(NSUInteger)index
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

- (ui::OpeningDays)unhandledDays
{
  return timeTableSet.GetUnhandledDays();
}

- (void)storeCachedData
{
  for (MWMOpeningHoursModel * m in self.sections)
    [m storeCachedData];
}

- (void)updateOpeningHours
{
  if (!self.isSimpleMode)
    return;
  std::stringstream sstr;
  sstr << MakeOpeningHours(timeTableSet).GetRule();
  self.delegate.openingHours = @(sstr.str().c_str());
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

- (UITableView *)tableView
{
  return self.delegate.tableView;
}

- (BOOL)isValid
{
  return osmoh::OpeningHours(self.delegate.openingHours.UTF8String).IsValid();
}

- (void)setIsSimpleMode:(BOOL)isSimpleMode
{
  id<MWMOpeningHoursModelProtocol> delegate = self.delegate;
  NSString * oh = delegate.openingHours;

  auto isSimple = isSimpleMode;
  if (isSimple && oh && oh.length)
    isSimple = MakeTimeTableSet(osmoh::OpeningHours(oh.UTF8String), timeTableSet);

  delegate.advancedEditor.hidden = isSimple;
  UITableView * tv = delegate.tableView;
  UIButton * toggleModeButton = delegate.toggleModeButton;

  if (isSimple)
  {
    _isSimpleMode = YES;
    tv.hidden = NO;
    [toggleModeButton setTitle:L(@"editor_time_advanced") forState:UIControlStateNormal];
    _sections = [NSMutableArray arrayWithCapacity:timeTableSet.Size()];
    while (self.sections.count < timeTableSet.Size())
      [self addSection];
    [tv reloadData];
  }
  else
  {
    if (_isSimpleMode)
    {
      [self updateOpeningHours];
      _isSimpleMode = NO;
    }
    tv.hidden = YES;
    [toggleModeButton setTitle:L(@"editor_time_simple") forState:UIControlStateNormal];
    MWMTextView * ev = delegate.editorView;
    ev.text = delegate.openingHours;
    [ev becomeFirstResponder];
  }
}

- (BOOL)isSimpleModeCapable
{
  NSString * oh = self.delegate.openingHours;
  if (!oh || !oh.length)
    return YES;
  ui::TimeTableSet tts;
  return MakeTimeTableSet(osmoh::OpeningHours(oh.UTF8String), tts);
}

@end
