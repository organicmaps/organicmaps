//
//  MWMGetTransitionMapAlert.m
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "ActiveMapsVC.h"
#import "MWMAlertViewController.h"
#import "MWMDownloaderDialogCell.h"
#import "MWMDownloaderDialogHeader.h"
#import "MWMDownloadTransitMapAlert.h"
#import "UIColor+MapsMeColor.h"
#import "UIKitCategories.h"
#import "UILabel+RuntimeAttributes.h"

typedef void (^MWMDownloaderBlock)();

static NSString * const kCellIdentifier = @"MWMDownloaderDialogCell";
static CGFloat const kCellHeight = 32.;
static CGFloat const kHeaderHeight = 43.;
static CGFloat const kHeaderAndFooterHeight = 44.;
static CGFloat const kMinimumOffset = 20.;

typedef NS_ENUM(NSUInteger, SelectionState)
{
  SelectionStateNone,
  SelectionStateMaps,
  SelectionStateRoutes,
  SelectionStateBoth
};

static NSString * const kDownloadTransitMapAlertNibName = @"MWMDownloadTransitMapAlert";

@interface MWMDownloadTransitMapAlert ()
{
  vector<storage::TIndex> maps;
  vector<storage::TIndex> routes;
}

@property (weak, nonatomic) IBOutlet UILabel * titleLabel;
@property (weak, nonatomic) IBOutlet UILabel * messageLabel;
@property (weak, nonatomic) IBOutlet UIButton * notNowButton;
@property (weak, nonatomic) IBOutlet UIButton * downloadButton;
@property (copy, nonatomic) MWMDownloaderBlock downloaderBlock;
@property (weak, nonatomic) IBOutlet UITableView * dialogsTableView;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * tableViewHeight;
@property (weak, nonatomic) IBOutlet UIView * divider;
@property (nonatomic) NSUInteger numberOfSections;

@property (nonatomic) MWMDownloaderDialogHeader * mapsHeader;
@property (nonatomic) MWMDownloaderDialogHeader * routesHeader;
@property (nonatomic) SelectionState state;

@end

@interface MWMDownloadTransitMapAlert (TableView) <UITableViewDataSource, UITableViewDelegate>

@end

@implementation MWMDownloadTransitMapAlert

+ (instancetype)crossCountryAlertWithMaps:(vector<storage::TIndex> const &)maps routes:(vector<storage::TIndex> const &)routes
{
  MWMDownloadTransitMapAlert * alert = [self alertWithMaps:maps routes:routes];
  alert.titleLabel.localizedText = @"dialog_routing_download_and_build_cross_route";
  alert.messageLabel.localizedText = @"dialog_routing_download_cross_route";
  return alert;
}

+ (instancetype)downloaderAlertWithMaps:(vector<storage::TIndex> const &)maps routes:(vector<storage::TIndex> const &)routes
{
  MWMDownloadTransitMapAlert * alert = [self alertWithMaps:maps routes:routes];
  alert.titleLabel.localizedText = @"dialog_routing_download_files";
  alert.messageLabel.localizedText = @"dialog_routing_download_and_update_all";
  return alert;
}

+ (instancetype)alertWithMaps:(vector<storage::TIndex> const &)maps routes:(vector<storage::TIndex> const &)routes
{
  MWMDownloadTransitMapAlert * alert = [[[NSBundle mainBundle] loadNibNamed:kDownloadTransitMapAlertNibName owner:nil options:nil] firstObject];
  alert->maps = maps;
  alert->routes = routes;
  alert.numberOfSections = 0;
  if (!maps.empty())
    alert.numberOfSections++;
  if (!routes.empty())
    alert.numberOfSections++;
  [alert configure];
  return alert;
}

- (void)configure
{
  [self.dialogsTableView registerNib:[UINib nibWithNibName:kCellIdentifier bundle:nil] forCellReuseIdentifier:kCellIdentifier];
  self.state = SelectionStateNone;
  if (maps.size() < 2 && routes.size() < 2)
    self.dialogsTableView.scrollEnabled = NO;
  __weak MWMDownloadTransitMapAlert * weakSelf = self;
  self.downloaderBlock = ^
  {
    __strong MWMDownloadTransitMapAlert * self = weakSelf;
    storage::Storage & s = GetFramework().Storage();
    for (auto const & index : maps)
      s.DownloadCountry(index, TMapOptions::EMapWithCarRouting);
    for (auto const & index : routes)
      s.DownloadCountry(index, TMapOptions::ECarRouting);
  };
  [self.dialogsTableView reloadData];
}

#pragma mark - Actions

- (IBAction)notNowButtonTap:(id)sender {
  [self close];
}

- (IBAction)downloadButtonTap:(id)sender {
  [self downloadMaps];
}

- (void)downloadMaps {
  self.downloaderBlock();
  [self close];
  ActiveMapsVC *activeMapsViewController = [[ActiveMapsVC alloc] init];
  [self.alertController.ownerViewController.navigationController pushViewController:activeMapsViewController animated:YES];
}

- (void)showDownloadDetail:(UIButton *)sender
{
  if ([sender isEqual:self.mapsHeader.headerButton])
  {
    if (!self.routesHeader.headerButton.selected)
      self.state = sender.selected ? SelectionStateMaps : SelectionStateNone;
    else
      self.state = sender.selected ? SelectionStateBoth : SelectionStateRoutes;
  }
  else
  {
    if (!self.mapsHeader.headerButton.selected)
      self.state = sender.selected ? SelectionStateRoutes : SelectionStateNone;
    else
      self.state = sender.selected ? SelectionStateBoth : SelectionStateMaps;
  }
}

- (void)setState:(SelectionState)state
{
  _state = state;
  [self layoutIfNeeded];
  switch (state)
  {
    case SelectionStateNone:
    {
      CGFloat const height = kHeaderAndFooterHeight * self.numberOfSections;
      self.tableViewHeight.constant = height;
      [self.dialogsTableView.visibleCells enumerateObjectsUsingBlock:^(MWMDownloaderDialogCell * obj, NSUInteger idx, BOOL *stop) {
        obj.titleLabel.alpha = 0.;
      }];
      [self.dialogsTableView beginUpdates];
      [self.dialogsTableView endUpdates];
      [UIView animateWithDuration:0.05 animations:^
      {
        [self layoutSubviews];
      }];
      break;
    }
    case SelectionStateMaps:
    case SelectionStateRoutes:
    case SelectionStateBoth:
    {
      NSUInteger const cellCount = self.cellCountForCurrentState;
      CGFloat const height = [self bounded:kCellHeight * cellCount + kHeaderAndFooterHeight * self.numberOfSections withHeight:self.superview.height];
      self.tableViewHeight.constant = height;
      [UIView animateWithDuration:.05 animations:^
      {
        [self layoutSubviews];
      }
      completion:^(BOOL finished)
      {
        [UIView animateWithDuration:.3 animations:^{
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
  switch (self.state)
  {
    case SelectionStateNone:
      self.tableViewHeight.constant = kHeaderAndFooterHeight * self.numberOfSections ;
      break;
    case SelectionStateMaps:
    case SelectionStateRoutes:
    case SelectionStateBoth:
    {
      NSUInteger const cellCount = self.cellCountForCurrentState;
      self.tableViewHeight.constant = [self bounded:(kCellHeight * cellCount + kHeaderAndFooterHeight * self.numberOfSections) withHeight:height];;
      break;
    }
  }
}

- (NSUInteger)cellCountForCurrentState
{
  if (self.state == SelectionStateBoth)
    return maps.size() + routes.size();
  else if (self.state == SelectionStateMaps)
    return maps.size();
  else
    return routes.size();
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
  return self.numberOfSections;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return section == 0 ? maps.size() : routes.size();
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
    case SelectionStateMaps:
      return indexPath.section == 0 ? kCellHeight : 0.;
    case SelectionStateRoutes:
      return indexPath.section == 0 ? 0. : kCellHeight;
    case SelectionStateBoth:
      return kCellHeight;
  }
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section
{
  storage::Storage & s = GetFramework().Storage();
  if (section == 0)
  {
    if (self.mapsHeader)
      return self.mapsHeader;

    if (maps.size() > 1)
    {
      uint64_t totalRoutingSize = 0;
      for (auto const & index : maps)
        totalRoutingSize += s.CountrySizeInBytes(index, TMapOptions::EMapWithCarRouting).second;

      NSString * size = [NSString stringWithFormat:@"%@ %@", @(totalRoutingSize / (1024 * 1024)), L(@"mb")];
      NSString * title = [NSString stringWithFormat:@"%@(%@)", L(@"dialog_routing_maps"), @(maps.size())];
      self.mapsHeader = [MWMDownloaderDialogHeader headerForOwnerAlert:self title:title size:size];
    }
    else
    {
      NSAssert(!maps.empty(), @"Maps can't be empty!");
      storage::TIndex const & index = maps[0];
      NSString * title = [NSString stringWithUTF8String:s.CountryName(index).c_str()];
      NSString * size = [NSString stringWithFormat:@"%@ %@", @(s.CountrySizeInBytes(index, TMapOptions::EMapWithCarRouting).second / (1024 * 1024)), L(@"mb")];
      self.mapsHeader = [MWMDownloaderDialogHeader headerForOwnerAlert:self title:title size:size];
      self.mapsHeader.expandImage.hidden = YES;
      self.mapsHeader.headerButton.enabled = NO;
      [self.mapsHeader layoutSizeLabel];
    }
    return self.mapsHeader;
  }

  if (self.routesHeader)
    return self.routesHeader;

  if (routes.size() > 1)
  {
    uint64_t totalRoutingSize = 0;
    for (auto const & index : routes)
      totalRoutingSize += s.CountrySizeInBytes(index, TMapOptions::ECarRouting).second;

    NSString * size = [NSString stringWithFormat:@"%@ %@", @(totalRoutingSize / (1024 * 1024)), L(@"mb")];
    NSString * title = [NSString stringWithFormat:@"%@(%@)", L(@"dialog_routing_routes_size"), @(routes.size())];
    self.routesHeader = [MWMDownloaderDialogHeader headerForOwnerAlert:self title:title size:size];
  }
  else
  {
    NSAssert(!routes.empty(), @"Routes can't be empty");
    storage::TIndex const & index = routes[0];
    NSString * title = [NSString stringWithUTF8String:s.CountryName(index).c_str()];
    NSString * size = [NSString stringWithFormat:@"%@ %@", @(s.CountrySizeInBytes(index, TMapOptions::ECarRouting).second / (1024 * 1024)), L(@"mb")];
    self.routesHeader = [MWMDownloaderDialogHeader headerForOwnerAlert:self title:title size:size];
    self.routesHeader.expandImage.hidden = YES;
    self.routesHeader.headerButton.enabled = NO;
    [self.routesHeader layoutSizeLabel];
  }
  return self.routesHeader;
}

- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section
{
  UIView * view = [[UIView alloc] init];
  view.backgroundColor = self.numberOfSections == 2 && section == 9 ? UIColor.blackDividers : UIColor.clearColor;
  return view;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  MWMDownloaderDialogCell * cell = (MWMDownloaderDialogCell *)[tableView dequeueReusableCellWithIdentifier:kCellIdentifier];
  if (!cell)
    cell = [[MWMDownloaderDialogCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:kCellIdentifier];
  storage::TIndex const & index = indexPath.section == 0 ? maps[indexPath.row] : routes[indexPath.row];
  storage::Storage & s = GetFramework().Storage();
  cell.titleLabel.text = [NSString stringWithUTF8String:s.CountryName(index).c_str()];
  return cell;
}

@end
