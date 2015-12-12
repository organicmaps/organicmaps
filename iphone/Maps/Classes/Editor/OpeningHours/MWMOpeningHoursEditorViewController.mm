#import "MWMOpeningHoursAddScheduleTableViewCell.h"
#import "MWMOpeningHoursEditorViewController.h"
#import "MWMOpeningHoursModel.h"
#import "MWMOpeningHoursSection.h"

extern NSDictionary * const kMWMOpeningHoursEditorTableCells = @{
  @(MWMOpeningHoursEditorDaysSelectorCell) : @"MWMOpeningHoursDaysSelectorTableViewCell",
  @(MWMOpeningHoursEditorAllDayCell) : @"MWMOpeningHoursAllDayTableViewCell",
  @(MWMOpeningHoursEditorTimeSpanCell) : @"MWMOpeningHoursTimeSpanTableViewCell",
  @(MWMOpeningHoursEditorTimeSelectorCell) : @"MWMOpeningHoursTimeSelectorTableViewCell",
  @(MWMOpeningHoursEditorClosedSpanCell) : @"MWMOpeningHoursClosedSpanTableViewCell",
  @(MWMOpeningHoursEditorAddClosedCell) : @"MWMOpeningHoursAddClosedTableViewCell",
  @(MWMOpeningHoursEditorDeleteScheduleCell) : @"MWMOpeningHoursDeleteScheduleTableViewCell",
  @(MWMOpeningHoursEditorSpacerCell) : @"MWMOpeningHoursSpacerTableViewCell",
  @(MWMOpeningHoursEditorAddScheduleCell) : @"MWMOpeningHoursAddScheduleTableViewCell",
};

@interface MWMOpeningHoursEditorViewController ()<UITableViewDelegate, UITableViewDataSource,
                                                  MWMOpeningHoursModelProtocol>

@property (weak, nonatomic, readwrite) IBOutlet UITableView * tableView;

@property (nonatomic) MWMOpeningHoursModel * model;

@end

@implementation MWMOpeningHoursEditorViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self configNavBar];
  [self configTable];
  [self configData];
}

#pragma mark - Configuration

- (void)configNavBar
{
  self.title = L(@"opening_hours");
  self.navigationItem.rightBarButtonItem =
      [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone
                                                    target:self
                                                    action:@selector(onDone)];
}

- (void)configTable
{
  [kMWMOpeningHoursEditorTableCells
      enumerateKeysAndObjectsUsingBlock:^(id _Nonnull key, NSString * identifier,
                                          BOOL * _Nonnull stop)
  {
    [self.tableView registerNib:[UINib nibWithNibName:identifier bundle:nil]
         forCellReuseIdentifier:identifier];
  }];
}

- (void)configData
{
  self.model = [[MWMOpeningHoursModel alloc] initWithDelegate:self];
}

#pragma mark - Actions

- (void)onDone
{
}

#pragma mark - Table

- (MWMOpeningHoursEditorCells)cellKeyForIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.section < self.model.count)
    return [self.model cellKeyForIndexPath:indexPath];
  else
    return MWMOpeningHoursEditorAddScheduleCell;
}

- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath
{
  NSString * identifier = kMWMOpeningHoursEditorTableCells[@([self cellKeyForIndexPath:indexPath])];
  NSAssert(identifier, @"Identifier can not be nil");
  return identifier;
}

- (CGFloat)heightForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  CGFloat const width = self.view.width;
  if (indexPath.section < self.model.count)
    return [self.model heightForIndexPath:indexPath withWidth:width];
  else
    return [MWMOpeningHoursAddScheduleTableViewCell height];
}

#pragma mark - Fill cells with data

- (void)fillCell:(MWMOpeningHoursTableViewCell * _Nonnull)cell atIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  if (indexPath.section < self.model.count)
    [self.model fillCell:cell atIndexPath:indexPath];
  else if ([cell isKindOfClass:[MWMOpeningHoursAddScheduleTableViewCell class]])
    ((MWMOpeningHoursAddScheduleTableViewCell *)cell).model = self.model;
}

#pragma mark - UITableViewDataSource

- (UITableViewCell * _Nonnull)tableView:(UITableView * _Nonnull)tableView cellForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  return [tableView dequeueReusableCellWithIdentifier:[self cellIdentifierForIndexPath:indexPath]];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView * _Nonnull)tableView
{
  return self.model.count + (self.model.canAddSection ? 1 : 0);
}

- (NSInteger)tableView:(UITableView * _Nonnull)tableView numberOfRowsInSection:(NSInteger)section
{
  return (section < self.model.count ? [self.model numberOfRowsInSection:section] : 1);
}

#pragma mark - UITableViewDelegate

- (CGFloat)tableView:(UITableView * _Nonnull)tableView heightForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  return [self heightForRowAtIndexPath:indexPath];
}

- (CGFloat)tableView:(UITableView * _Nonnull)tableView estimatedHeightForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  return [self heightForRowAtIndexPath:indexPath];
}

- (void)tableView:(UITableView * _Nonnull)tableView willDisplayCell:(MWMOpeningHoursTableViewCell * _Nonnull)cell forRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  [self fillCell:cell atIndexPath:indexPath];
}

@end
