#import "ActiveMapsVC.h"
#import "Common.h"
#import "MWMAlertViewController.h"
#import "MWMDownloaderDialogCell.h"
#import "MWMDownloaderDialogHeader.h"
#import "MWMDownloadTransitMapAlert.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"
#import "UILabel+RuntimeAttributes.h"

@interface MWMDownloaderEntity : NSObject 

@property (copy, nonatomic) NSArray * titles;
@property (copy, nonatomic) NSString * size;
@property (nonatomic) BOOL isMapsFiles;

- (instancetype)initWithIndexes:(vector<storage::TIndex> const &)indexes isMaps:(BOOL)isMaps;

@end

@implementation MWMDownloaderEntity

- (instancetype)initWithIndexes:(vector<storage::TIndex> const &)indexes isMaps:(BOOL)isMaps
{
  self = [super init];
  if (self)
  {
    auto & a = GetFramework().GetCountryTree().GetActiveMapLayout();
    NSMutableArray * titles = [@[] mutableCopy];
    uint64_t totalRoutingSize = 0;
    for (auto const & i : indexes)
    {
      [titles addObject:@(a.GetCountryName(i).c_str())];
      totalRoutingSize += a.GetCountrySize(i, isMaps ? MapOptions::MapWithCarRouting : MapOptions::CarRouting).second;
    }
    self.isMapsFiles = isMaps;
    self.titles = titles;
    self.size = [NSString stringWithFormat:@"%@ %@", @(totalRoutingSize / MB), L(@"mb")];
  }
  return self;
}

@end

typedef void (^MWMDownloaderBlock)();

typedef NS_ENUM(NSUInteger, SelectionState)
{
  SelectionStateNone,
  SelectionStateFirst,
  SelectionStateSecond,
  SelectionStateBoth
};

static NSString * const kCellIdentifier = @"MWMDownloaderDialogCell";
static NSString * const kDownloadTransitMapAlertNibName = @"MWMDownloadTransitMapAlert";
static CGFloat const kCellHeight = 32.;
static CGFloat const kHeaderHeight = 43.;
static CGFloat const kHeaderAndFooterHeight = 44.;
static CGFloat const kMinimumOffset = 20.;

static NSString * const kStatisticsEvent = @"Map download Alert";

@interface MWMDownloadTransitMapAlert ()
{
  vector<storage::TIndex> maps;
  vector<storage::TIndex> routes;
}

@property (weak, nonatomic) IBOutlet UILabel * titleLabel;
@property (weak, nonatomic) IBOutlet UILabel * messageLabel;
@property (copy, nonatomic) MWMDownloaderBlock downloaderBlock;
@property (weak, nonatomic) IBOutlet UITableView * dialogsTableView;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * tableViewHeight;
@property (copy, nonatomic) NSArray * missedFiles;
@property (nonatomic) MWMDownloaderDialogHeader * firstHeader;
@property (nonatomic) MWMDownloaderDialogHeader * secondHeader;
@property (nonatomic) SelectionState state;

@end

@interface MWMDownloadTransitMapAlert (TableView) <UITableViewDataSource, UITableViewDelegate>

@end

@implementation MWMDownloadTransitMapAlert

+ (instancetype)downloaderAlertWithMaps:(vector<storage::TIndex> const &)maps
                                 routes:(vector<storage::TIndex> const &)routes
                                   code:(routing::IRouter::ResultCode)code
{
  [[Statistics instance] logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatOpen}];
  MWMDownloadTransitMapAlert * alert = [self alertWithMaps:maps routes:routes];
  switch (code)
  {
    case routing::IRouter::InconsistentMWMandRoute:
    case routing::IRouter::RouteNotFound:
    case routing::IRouter::RouteFileNotExist:
      alert.titleLabel.localizedText = @"dialog_routing_download_files";
      alert.messageLabel.localizedText = @"dialog_routing_download_and_update_all";
      break;
    case routing::IRouter::FileTooOld:
      alert.titleLabel.localizedText = @"dialog_routing_download_files";
      alert.messageLabel.localizedText = @"dialog_routing_download_and_update_maps";
      break;
    case routing::IRouter::NeedMoreMaps:
      alert.titleLabel.localizedText = @"dialog_routing_download_and_build_cross_route";
      alert.messageLabel.localizedText = @"dialog_routing_download_cross_route";
      break;
    default:
      NSAssert(false, @"Incorrect code!");
      break;
  }
  return alert;
}

+ (instancetype)alertWithMaps:(vector<storage::TIndex> const &)maps routes:(vector<storage::TIndex> const &)routes
{
  MWMDownloadTransitMapAlert * alert = [[[NSBundle mainBundle] loadNibNamed:kDownloadTransitMapAlertNibName owner:nil options:nil] firstObject];
  NSMutableArray * missedFiles = [@[] mutableCopy];
  if (!maps.empty())
  {
    MWMDownloaderEntity * entity = [[MWMDownloaderEntity alloc] initWithIndexes:maps isMaps:YES];
    [missedFiles addObject:entity];
  }
  if (!routes.empty())
  {
    MWMDownloaderEntity * entity = [[MWMDownloaderEntity alloc] initWithIndexes:routes isMaps:NO];
    [missedFiles addObject:entity];
  }
  alert.missedFiles = missedFiles;
  alert->maps = maps;
  alert->routes = routes;
  __weak MWMDownloadTransitMapAlert * wAlert = alert;
  alert.downloaderBlock = ^()
  {
    __strong MWMDownloadTransitMapAlert * alert = wAlert;
    auto & a = GetFramework().GetCountryTree().GetActiveMapLayout();
    for (auto const & index : alert->maps)
      a.DownloadMap(index, MapOptions::MapWithCarRouting);
    for (auto const & index : alert->routes)
      a.DownloadMap(index, MapOptions::CarRouting);
  };
  [alert configure];
  return alert;
}

- (void)configure
{
  [self.dialogsTableView registerNib:[UINib nibWithNibName:kCellIdentifier bundle:nil] forCellReuseIdentifier:kCellIdentifier];
  self.state = SelectionStateNone;
  [self.dialogsTableView reloadData];
}

#pragma mark - Actions

- (IBAction)notNowButtonTap:(id)sender
{
  [[Statistics instance] logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatClose}];
  [self close];
}

- (IBAction)downloadButtonTap:(id)sender
{
  [[Statistics instance] logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatApply}];
  [self downloadMaps];
}

- (void)downloadMaps
{
  self.downloaderBlock();
  [self close];
  ActiveMapsVC * activeMapsViewController = [[ActiveMapsVC alloc] init];
  [self.alertController.ownerViewController.navigationController pushViewController:activeMapsViewController animated:YES];
}

- (void)showDownloadDetail:(UIButton *)sender
{
  if ([sender isEqual:self.firstHeader.headerButton])
  {
    if (!self.secondHeader.headerButton.selected)
      self.state = sender.selected ? SelectionStateFirst : SelectionStateNone;
    else
      self.state = sender.selected ? SelectionStateBoth : SelectionStateSecond;
  }
  else
  {
    if (!self.firstHeader.headerButton.selected)
      self.state = sender.selected ? SelectionStateSecond : SelectionStateNone;
    else
      self.state = sender.selected ? SelectionStateBoth : SelectionStateFirst;
  }
}

- (void)setState:(SelectionState)state
{
  _state = state;
  [self layoutIfNeeded];
  NSUInteger const numberOfSections = self.missedFiles.count;
  switch (state)
  {
    case SelectionStateNone:
    {
      CGFloat const height = kHeaderAndFooterHeight * numberOfSections;
      self.tableViewHeight.constant = height;
      self.dialogsTableView.scrollEnabled = NO;
      [self.dialogsTableView.visibleCells enumerateObjectsUsingBlock:^(MWMDownloaderDialogCell * obj, NSUInteger idx, BOOL *stop) {
        obj.titleLabel.alpha = 0.;
      }];
      [self.dialogsTableView beginUpdates];
      [self.dialogsTableView endUpdates];
      [UIView animateWithDuration:.05 animations:^
      {
        [self layoutSubviews];
      }];
      break;
    }
    case SelectionStateFirst:
    case SelectionStateSecond:
    case SelectionStateBoth:
    {
      NSUInteger const cellCount = self.cellCountForCurrentState;
      CGFloat const actualHeight = kCellHeight * cellCount + kHeaderAndFooterHeight * numberOfSections;
      CGFloat const height = [self bounded:actualHeight withHeight:self.superview.height];
      self.tableViewHeight.constant = height;
      self.dialogsTableView.scrollEnabled = actualHeight > self.tableViewHeight.constant ? YES : NO;
      [UIView animateWithDuration:.05 animations:^
      {
        [self layoutSubviews];
      }
      completion:^(BOOL finished)
      {
        [UIView animateWithDuration:kDefaultAnimationDuration animations:^{
          [self.dialogsTableView beginUpdates];
          [self.dialogsTableView.visibleCells enumerateObjectsUsingBlock:^(MWMDownloaderDialogCell * obj, NSUInteger idx, BOOL *stop)
            {
              obj.titleLabel.alpha = 1.;
          }];
          [self.dialogsTableView endUpdates];
        }];
      }];
      break;
    }
  }
}

- (CGFloat)bounded:(CGFloat)f withHeight:(CGFloat)h
{
  CGFloat const currentHeight = [self.subviews.firstObject height];
  CGFloat const maximumHeight = h - 2. * kMinimumOffset;
  CGFloat const availableHeight = maximumHeight - currentHeight;
  return MIN(f, availableHeight + self.tableViewHeight.constant);
}

- (void)invalidateTableConstraintWithHeight:(CGFloat)height
{
  NSUInteger const numberOfSections = self.missedFiles.count;
  switch (self.state)
  {
    case SelectionStateNone:
      self.tableViewHeight.constant = kHeaderAndFooterHeight * numberOfSections ;
      break;
    case SelectionStateFirst:
    case SelectionStateSecond:
    case SelectionStateBoth:
    {
      NSUInteger const cellCount = self.cellCountForCurrentState;
      CGFloat const actualHeight = kCellHeight * cellCount + kHeaderAndFooterHeight * numberOfSections;
      self.tableViewHeight.constant = [self bounded:actualHeight withHeight:height];;
      self.dialogsTableView.scrollEnabled = actualHeight > self.tableViewHeight.constant;
      break;
    }
  }
}

- (NSUInteger)cellCountForCurrentState
{
  if (self.state == SelectionStateBoth)
    return [[self.missedFiles[0] titles] count] + [[self.missedFiles[1] titles] count];
  else if (self.state == SelectionStateFirst)
    return [[self.missedFiles[0] titles] count];
  else
    return [[self.missedFiles[1] titles] count];
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
  CGFloat const height = UIInterfaceOrientationIsLandscape(orientation) ? MIN(self.superview.width, self.superview.height) : MAX(self.superview.width, self.superview.height);
  [self invalidateTableConstraintWithHeight:height];
}

@end

@implementation MWMDownloadTransitMapAlert (TableView)

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return self.missedFiles.count;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return [[self.missedFiles[section] titles] count];
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
{
  return kHeaderHeight;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  switch (self.state)
  {
    case SelectionStateNone:
      return 0.;
    case SelectionStateFirst:
      return indexPath.section == 0 ? kCellHeight : 0.;
    case SelectionStateSecond:
      return indexPath.section == 0 ? 0. : kCellHeight;
    case SelectionStateBoth:
      return kCellHeight;
  }
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section
{
  if (section == 0)
  {
    if (self.firstHeader)
      return self.firstHeader;
  }
  else
  {
    if (self.secondHeader)
      return self.secondHeader;
  }

  MWMDownloaderEntity * entity = self.missedFiles[section];
  NSUInteger const count = entity.titles.count;
  NSString * title;
  MWMDownloaderDialogHeader * header;
  if (count > 1)
  {
    title = entity.isMapsFiles ? [NSString stringWithFormat:@"%@ (%@)", L(@"maps"), @(count)] : [NSString stringWithFormat:@"%@(%@)", L(@"dialog_routing_routes_size"), @(count)];;
    header = [MWMDownloaderDialogHeader headerForOwnerAlert:self title:title size:entity.size];
  }
  else
  {
    title = entity.titles.firstObject;
    header = [MWMDownloaderDialogHeader headerForOwnerAlert:self title:title size:entity.size];
    header.headerButton.enabled = NO;
    header.expandImage.hidden = YES;
    [header layoutSizeLabel];
  }
  if (section == 0)
  {
    self.firstHeader = header;
    return self.firstHeader;
  }
  self.secondHeader = header;
  return self.secondHeader;
}

- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section
{
  UIView * view = [[UIView alloc] init];
  view.backgroundColor = self.missedFiles.count == 2 && section == 0 ? UIColor.blackDividers : UIColor.blackOpaque;
  return view;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  MWMDownloaderDialogCell * cell = (MWMDownloaderDialogCell *)[tableView dequeueReusableCellWithIdentifier:kCellIdentifier];
  if (!cell)
    cell = [[MWMDownloaderDialogCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:kCellIdentifier];

  NSString * title = [self.missedFiles[indexPath.section] titles][indexPath.row];
  cell.titleLabel.text = title;
  return cell;
}

@end
