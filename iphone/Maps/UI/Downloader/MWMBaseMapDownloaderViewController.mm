#import "MWMBaseMapDownloaderViewController.h"
#import "MWMButton.h"
#import "MWMCommon.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMMapDownloaderAdsTableViewCell.h"
#import "MWMMapDownloaderCellHeader.h"
#import "MWMMapDownloaderDefaultDataSource.h"
#import "MWMMapDownloaderExtendedDataSourceWithAds.h"
#import "MWMMapDownloaderLargeCountryTableViewCell.h"
#import "MWMMapDownloaderPlaceTableViewCell.h"
#import "MWMMapDownloaderSubplaceTableViewCell.h"
#import "MWMMapDownloaderTableViewCell.h"
#import "MWMMigrationViewController.h"
#import "MWMMyTarget.h"
#import "MWMSegue.h"
#import "MWMStorage.h"
#import "MWMToast.h"
#import "SwiftBridge.h"
#import "UIViewController+Navigation.h"

#include "Framework.h"

namespace
{

typedef NS_OPTIONS(NSUInteger, ActionButtons)
{
  NoAction             = 0,
  ShowOnMapAction      = 1 << 1,
  DownloadAction       = 1 << 2,
  UpdateAction         = 1 << 3,
  CancelDownloadAction = 1 << 4,
  RetryDownloadAction  = 1 << 5,
  DeleteAction         = 1 << 6
};

NSString * const kAllMapsLabelFormat = @"%@ (%@)";
NSString * const kCancelActionTitle = L(@"cancel");
NSString * const kCancelAllTitle = L(@"downloader_cancel_all");
NSString * const kCancelDownloadActionTitle = L(@"cancel_download");
NSString * const kDeleteActionTitle = L(@"downloader_delete_map");
NSString * const kDownloadActionTitle = L(@"downloader_download_map");
NSString * const kDownloadAllActionTitle = L(@"downloader_download_all_button");
NSString * const kDownloadingTitle = L(@"downloader_downloading");
NSString * const kMapsTitle = L(@"downloader_maps");
NSString * const kRetryActionTitle = L(@"downloader_retry");
NSString * const kShowOnMapActionTitle = L(@"zoom_to_country");
NSString * const kUpdateActionTitle = L(@"downloader_status_outdated");
NSString * const kUpdateAllTitle = L(@"downloader_update_all_button");

NSString * const kBaseControllerIdentifier = @"MWMBaseMapDownloaderViewController";
NSString * const kControllerIdentifier = @"MWMMapDownloaderViewController";
} // namespace

using namespace storage;

@interface MWMBaseMapDownloaderViewController ()<UIScrollViewDelegate, MWMFrameworkStorageObserver,
                                                 MWMMyTargetDelegate>

@property (weak, nonatomic) IBOutlet UITableView * tableView;

@property (weak, nonatomic) IBOutlet UIView * allMapsView;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * allMapsViewBottomOffset;
@property (weak, nonatomic) IBOutlet MWMButton * allMapsButton;
@property (weak, nonatomic) IBOutlet MWMButton * allMapsCancelButton;

@property (nonatomic) UIImage * navBarBackground;
@property (nonatomic) UIImage * navBarShadow;

@property (nonatomic) CGFloat lastScrollOffset;

@property (nonatomic) MWMMapDownloaderDataSource * dataSource;
@property (nonatomic) MWMMapDownloaderDefaultDataSource * defaultDataSource;

@property (nonatomic) BOOL skipCountryEventProcessing;
@property (nonatomic) BOOL forceFullReload;

@property (nonatomic, readonly) NSString * parentCountryId;
@property(nonatomic, readonly) MWMMapDownloaderMode mode;

@property (nonatomic) BOOL showAllMapsButtons;

@end

@implementation MWMBaseMapDownloaderViewController
{
  TCountryId m_actionSheetId;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self configNavBar];
  [self configTable];
  [self configMyTarget];
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  UINavigationBar * navBar = [UINavigationBar appearance];
  self.navBarBackground = [navBar backgroundImageForBarMetrics:UIBarMetricsDefault];
  self.navBarShadow = navBar.shadowImage;
  UIColor * searchBarColor = [UIColor primary];
  [navBar setBackgroundImage:[UIImage imageWithColor:searchBarColor]
               forBarMetrics:UIBarMetricsDefault];
  navBar.shadowImage = [[UIImage alloc] init];
  [MWMFrameworkListener addObserver:self];
  [self configViews];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [super viewWillDisappear:animated];
  UINavigationBar * navBar = [UINavigationBar appearance];
  [navBar setBackgroundImage:self.navBarBackground forBarMetrics:UIBarMetricsDefault];
  navBar.shadowImage = self.navBarShadow;
  [MWMFrameworkListener removeObserver:self];
  [self notifyParentController];
}

- (void)configViews
{
  [self configAllMapsView];
}

- (void)configNavBar
{
  BOOL const downloaded = self.mode == MWMMapDownloaderModeDownloaded;
  if (self.dataSource.isParentRoot)
  {
    self.title = downloaded ? L(@"downloader_my_maps_title") : L(@"download_maps");
  }
  else
  {
    NodeAttrs nodeAttrs;
    GetFramework().GetStorage().GetNodeAttrs(self.parentCountryId.UTF8String, nodeAttrs);
    self.title = @(nodeAttrs.m_nodeLocalName.c_str());
  }

  if (downloaded)
  {
    UIBarButtonItem * addButton = [self buttonWithImage:[UIImage imageNamed:@"ic_nav_bar_add"]
                                                 action:@selector(openAvailableMaps)];
    self.navigationItem.rightBarButtonItems = [self alignedNavBarButtonItems:@[ addButton ]];
  }
}

- (void)configMyTarget { [MWMMyTarget manager].delegate = self; }

- (void)backTap
{
  UINavigationController * navVC = self.navigationController;
  NSArray<UIViewController *> * viewControllers = navVC.viewControllers;
  NSInteger const viewControllersCount = viewControllers.count;
  NSInteger const prevVCIndex = viewControllersCount - 2;
  Class const migrationClass = [MWMMigrationViewController class];
  if (prevVCIndex < 0 || [viewControllers[prevVCIndex] isKindOfClass:migrationClass])
    [navVC popToRootViewControllerAnimated:YES];
  else
    [super backTap];
}

- (void)notifyParentController
{
  NSArray<MWMViewController *> * viewControllers = [self.navigationController viewControllers];
  BOOL const goingTreeDeeper = ([viewControllers indexOfObject:self] != NSNotFound);
  if (goingTreeDeeper)
    return;
  MWMViewController * parentVC = viewControllers.lastObject;
  if ([parentVC isKindOfClass:[MWMBaseMapDownloaderViewController class]])
  {
    MWMBaseMapDownloaderViewController * downloaderVC = static_cast<MWMBaseMapDownloaderViewController *>(parentVC);
    [downloaderVC processCountryEvent:self.parentCountryId.UTF8String];
  }
}

#pragma mark - MWMFrameworkStorageObserver

- (void)processCountryEvent:(TCountryId const &)countryId
{
  if (self.skipCountryEventProcessing)
    return;

  for (UITableViewCell * cell in self.tableView.visibleCells)
  {
    if ([cell conformsToProtocol:@protocol(MWMFrameworkStorageObserver)])
      [static_cast<id<MWMFrameworkStorageObserver>>(cell) processCountryEvent:countryId];
  }

  BOOL needReload = NO;
  auto const & s = GetFramework().GetStorage();
  s.ForEachInSubtree(self.parentCountryId.UTF8String,
                     [&needReload, &countryId](TCountryId const & descendantId, bool groupNode)
                     {
                       needReload = needReload || countryId == descendantId;
                     });
  if (needReload)
  {
    [self configViews];
    [self reloadData];
  }
}

- (void)processCountry:(TCountryId const &)countryId progress:(MapFilesDownloader::TProgress const &)progress
{
  for (UITableViewCell * cell in self.tableView.visibleCells)
  {
    if ([cell conformsToProtocol:@protocol(MWMFrameworkStorageObserver)])
      [static_cast<id<MWMFrameworkStorageObserver>>(cell) processCountry:countryId progress:progress];
  }
}

#pragma mark - Table

- (void)configTable
{
  UITableView * tv = self.tableView;
  tv.separatorColor = [UIColor blackDividers];
  [tv registerWithCellClass:[MWMMapDownloaderAdsTableViewCell class]];
  [tv registerWithCellClass:[MWMMapDownloaderButtonTableViewCell class]];
  [tv registerWithCellClass:[MWMMapDownloaderTableViewCell class]];
  [tv registerWithCellClass:[MWMMapDownloaderLargeCountryTableViewCell class]];
  [tv registerWithCellClass:[MWMMapDownloaderPlaceTableViewCell class]];
  [tv registerWithCellClass:[MWMMapDownloaderSubplaceTableViewCell class]];
}

#pragma mark - MWMMyTargetDelegate

- (void)onAppWallRefresh { [self reloadTable]; }
#pragma mark - UIScrollViewDelegate

- (void)scrollViewWillEndDragging:(UIScrollView *)scrollView withVelocity:(CGPoint)velocity targetContentOffset:(inout CGPoint *)targetContentOffset
{
  SEL const selector = @selector(refreshAllMapsView);
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:selector object:nil];
  [self refreshAllMapsViewForOffset:targetContentOffset->y];
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView
{
  SEL const selector = @selector(refreshAllMapsView);
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:selector object:nil];
  [self performSelector:selector withObject:nil afterDelay:kDefaultAnimationDuration];
}

#pragma mark - All Maps Action

- (void)configAllMapsView
{
  auto const & s = GetFramework().GetStorage();
  TCountryId const parentCountryId = self.parentCountryId.UTF8String;
  if (self.dataSource != self.defaultDataSource)
  {
    self.showAllMapsButtons = NO;
  }
  else if (self.mode == MWMMapDownloaderModeDownloaded)
  {
    Storage::UpdateInfo updateInfo{};
    s.GetUpdateInfo(parentCountryId, updateInfo);
    self.showAllMapsButtons = updateInfo.m_numberOfMwmFilesToUpdate != 0;
    if (self.showAllMapsButtons)
    {
      self.allMapsButton.hidden = NO;
      [self.allMapsButton
          setTitle:[NSString stringWithFormat:kAllMapsLabelFormat, kUpdateAllTitle,
                                              formattedSize(updateInfo.m_totalUpdateSizeInBytes)]
          forState:UIControlStateNormal];
      self.allMapsCancelButton.hidden = YES;
    }
  }
  else if (parentCountryId != s.GetRootId())
  {
    TCountriesVec queuedChildren;
    s.GetQueuedChildren(parentCountryId, queuedChildren);
    if (queuedChildren.empty())
    {
      TCountriesVec downloadedChildren;
      TCountriesVec availableChildren;
      s.GetChildrenInGroups(parentCountryId, downloadedChildren, availableChildren, true /* keepAvailableChildren */);
      self.showAllMapsButtons = downloadedChildren.size() != availableChildren.size();
      if (self.showAllMapsButtons)
      {
        NodeAttrs nodeAttrs;
        s.GetNodeAttrs(parentCountryId, nodeAttrs);
        self.allMapsButton.hidden = NO;
        [self.allMapsButton
         setTitle:[NSString stringWithFormat:kAllMapsLabelFormat, kDownloadAllActionTitle,
                   formattedSize(nodeAttrs.m_mwmSize)]
         forState:UIControlStateNormal];
        self.allMapsCancelButton.hidden = YES;
      }
    }
    else
    {
      TMwmSize queuedSize = 0;
      for (TCountryId const & countryId : queuedChildren)
      {
        NodeAttrs nodeAttrs;
        s.GetNodeAttrs(countryId, nodeAttrs);
        queuedSize += nodeAttrs.m_mwmSize;
      }
      self.showAllMapsButtons = YES;
      self.allMapsButton.hidden = YES;
      self.allMapsCancelButton.hidden = NO;
      [self.allMapsCancelButton
       setTitle:[NSString stringWithFormat:kAllMapsLabelFormat, kCancelAllTitle,
                 formattedSize(queuedSize)]
       forState:UIControlStateNormal];
    }
  }
  else
  {
    self.showAllMapsButtons = NO;
  }
}

- (void)refreshAllMapsView
{
  [self refreshAllMapsViewForOffset:self.tableView.contentOffset.y];
}

- (void)refreshAllMapsViewForOffset:(CGFloat)scrollOffset
{
  if (!self.showAllMapsButtons)
    return;
  UITableView * tv = self.tableView;
  CGFloat const bottomOffset = tv.contentSize.height - tv.frame.size.height;
  BOOL const hide =
      scrollOffset >= self.lastScrollOffset && scrollOffset > 0 && scrollOffset < bottomOffset;
  self.lastScrollOffset = scrollOffset;
  if (self.allMapsView.hidden == hide)
    return;
  if (!hide)
    self.allMapsView.hidden = hide;
  [self.view layoutIfNeeded];
  self.allMapsViewBottomOffset.constant = hide ? self.allMapsView.height : 0.0;
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.allMapsView.alpha = hide ? 0.0 : 1.0;
    [self.view layoutIfNeeded];
  }
  completion:^(BOOL finished)
  {
    if (hide)
      self.allMapsView.hidden = hide;
  }];
}

- (IBAction)allMapsAction
{
  self.skipCountryEventProcessing = YES;
  TCountryId const parentCountryId = self.parentCountryId.UTF8String;
  if (self.mode == MWMMapDownloaderModeDownloaded)
  {
    [Statistics logEvent:kStatDownloaderMapAction
          withParameters:@{
            kStatAction : kStatUpdate,
            kStatIsAuto : kStatNo,
            kStatFrom : kStatDownloader,
            kStatScenario : kStatUpdateAll
          }];
    [MWMStorage updateNode:parentCountryId];
  }
  else
  {
    [Statistics logEvent:kStatDownloaderMapAction
          withParameters:@{
                           kStatAction : kStatDownload,
                           kStatIsAuto : kStatNo,
                           kStatFrom : kStatDownloader,
                           kStatScenario : kStatDownloadGroup
                           }];
    [MWMStorage downloadNode:parentCountryId
                   onSuccess:nil];
  }
  self.skipCountryEventProcessing = NO;
  [self processCountryEvent:parentCountryId];
}

- (IBAction)allMapsCancelAction
{
  self.skipCountryEventProcessing = YES;
  [Statistics logEvent:kStatDownloaderMapAction
        withParameters:@{
                         kStatAction : kStatCancel,
                         kStatIsAuto : kStatNo,
                         kStatFrom : kStatDownloader,
                         kStatScenario : kStatDownloadGroup
                         }];
  TCountryId const parentCountryId = self.parentCountryId.UTF8String;
  [MWMStorage cancelDownloadNode:parentCountryId];
  self.skipCountryEventProcessing = NO;
  [self processCountryEvent:parentCountryId];
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
  Class cls = [self.dataSource cellClassForIndexPath:indexPath];
  if ([MWMMapDownloaderLargeCountryTableViewCell class] == cls)
  {
    NSAssert(self.dataSource != nil, @"Datasource is nil.");
    NSString * countyId = [self.dataSource countryIdForIndexPath:indexPath];
    NSAssert(countyId != nil, @"CountryId is nil.");
    [self openNodeSubtree:countyId.UTF8String];
  }
  else if ([MWMMapDownloaderAdsTableViewCell class] == cls)
  {
    [[MWMMyTarget manager] handleBannerClickAtIndex:indexPath.row withController:self];
  }
  else
  {
    [self showActionSheetForRowAtIndexPath:indexPath];
  }
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  Class cls = [self.dataSource cellClassForIndexPath:indexPath];
  if ([MWMMapDownloaderAdsTableViewCell class] == cls)
  {
    MWMMapDownloaderExtendedDataSourceWithAds * adDataSource =
        static_cast<MWMMapDownloaderExtendedDataSourceWithAds *>(self.dataSource);
    MTRGAppwallBannerAdView * adView = [adDataSource viewForBannerAtIndex:indexPath.row];
    return [adView sizeThatFits:{CGRectGetWidth(tableView.bounds), 0}].height + 1;
  }
  return UITableViewAutomaticDimension;
}

- (CGFloat)tableView:(UITableView *)tableView estimatedHeightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if ([self.dataSource isButtonCell:indexPath.section])
    return [MWMMapDownloaderButtonTableViewCell estimatedHeight];
  Class<MWMMapDownloaderTableViewCellProtocol> cellClass =
      [self.dataSource cellClassForIndexPath:indexPath];
  return [cellClass estimatedHeight];
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
{
  return [self.dataSource isButtonCell:section] ? 36 : 28;
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section
{
  return section == [tableView numberOfSections] - 1 ? 68 : 0;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section
{
  CGFloat const width = CGRectGetWidth(tableView.bounds);
  CGFloat const height = [self tableView:tableView heightForHeaderInSection:section];
  MWMMapDownloaderCellHeader * headerView =
      [[MWMMapDownloaderCellHeader alloc] initWithFrame:{{}, {width, height}}];
  headerView.text = [self.dataSource tableView:tableView titleForHeaderInSection:section];
  return headerView;
}

#pragma mark - UILongPressGestureRecognizer

- (IBAction)longPress:(UILongPressGestureRecognizer *)sender
{
  if (sender.state != UIGestureRecognizerStateBegan)
    return;
  NSIndexPath * indexPath = [self.tableView indexPathForRowAtPoint:[sender locationInView:self.tableView]];
  if (!indexPath)
    return;
  if ([self.dataSource isButtonCell:indexPath.section])
    return;
  Class cls = [self.dataSource cellClassForIndexPath:indexPath];
  if ([MWMMapDownloaderAdsTableViewCell class] == cls)
    return;
  [self showActionSheetForRowAtIndexPath:indexPath];
}

#pragma mark - Action Sheet

- (void)showActionSheetForRowAtIndexPath:(NSIndexPath *)indexPath
{
  auto const & s = GetFramework().GetStorage();
  NodeAttrs nodeAttrs;
  NSAssert(self.dataSource != nil, @"Datasource is nil.");
  NSString * countyId = [self.dataSource countryIdForIndexPath:indexPath];
  NSAssert(countyId != nil, @"CountryId is nil.");
  m_actionSheetId = countyId.UTF8String;
  s.GetNodeAttrs(m_actionSheetId, nodeAttrs);

  ActionButtons buttons = NoAction;
  switch (nodeAttrs.m_status)
  {
    case NodeStatus::Undefined:
      break;
    case NodeStatus::NotDownloaded:
      buttons |= DownloadAction;
      break;
    case NodeStatus::Downloading:
    case NodeStatus::InQueue:
      buttons |= CancelDownloadAction;
      break;
    case NodeStatus::OnDiskOutOfDate:
      buttons |= ShowOnMapAction;
      buttons |= UpdateAction;
      buttons |= DeleteAction;
      break;
    case NodeStatus::OnDisk:
      buttons |= ShowOnMapAction;
      buttons |= DeleteAction;
      break;
    case NodeStatus::Partly:
      buttons |= DownloadAction;
      buttons |= DeleteAction;
      break;
    case NodeStatus::Error:
      buttons |= RetryDownloadAction;
      if (nodeAttrs.m_localMwmCounter != 0)
        buttons |= DeleteAction;
      break;
  }

  NSAssert(buttons != NoAction, @"No action buttons defined.");
  if (buttons == NoAction)
    return;

  UITableViewCell * cell = [self.tableView cellForRowAtIndexPath:indexPath];
  UIView * cellSuperView = cell.superview;
  if (!cellSuperView)
    return;

  NSString * title = @(nodeAttrs.m_nodeLocalName.c_str());
  BOOL const isMultiParent = (nodeAttrs.m_parentInfo.size() > 1);
  NSString * message = (self.dataSource.isParentRoot || isMultiParent)
                           ? nil
                           : @(nodeAttrs.m_parentInfo[0].m_localName.c_str());

  UIAlertController * alertController =
      [UIAlertController alertControllerWithTitle:title
                                          message:message
                                   preferredStyle:UIAlertControllerStyleActionSheet];
  alertController.popoverPresentationController.sourceView = cell;
  alertController.popoverPresentationController.sourceRect = cell.bounds;
  [self addButtons:buttons toActionComponent:alertController];
  [self presentViewController:alertController animated:YES completion:nil];
}

- (void)addButtons:(ActionButtons)buttons toActionComponent:(UIAlertController *)alertController
{
  if (buttons & ShowOnMapAction)
  {
    UIAlertAction * action = [UIAlertAction actionWithTitle:kShowOnMapActionTitle
                                                      style:UIAlertActionStyleDefault
                                                    handler:^(UIAlertAction * action)
                              { [self showNode:self->m_actionSheetId]; }];
    [alertController addAction:action];
  }

  auto const & s = GetFramework().GetStorage();
  if (buttons & DownloadAction)
  {
    NodeAttrs nodeAttrs;
    s.GetNodeAttrs(m_actionSheetId, nodeAttrs);
    NSString * prefix = nodeAttrs.m_mwmCounter == 1 ? kDownloadActionTitle : kDownloadAllActionTitle;
    NSString * title = [NSString stringWithFormat:kAllMapsLabelFormat, prefix,
                        formattedSize(nodeAttrs.m_mwmSize)];
    UIAlertAction * action = [UIAlertAction actionWithTitle:title
                                                      style:UIAlertActionStyleDefault
                                                    handler:^(UIAlertAction * action)
                              { [self downloadNode:self->m_actionSheetId]; }];
    [alertController addAction:action];
  }

  if (buttons & UpdateAction)
  {
    Storage::UpdateInfo updateInfo;
    s.GetUpdateInfo(m_actionSheetId, updateInfo);
    NSString * title = [NSString stringWithFormat:kAllMapsLabelFormat, kUpdateActionTitle,
                        formattedSize(updateInfo.m_totalUpdateSizeInBytes)];
    UIAlertAction * action = [UIAlertAction actionWithTitle:title
                                                      style:UIAlertActionStyleDefault
                                                    handler:^(UIAlertAction * action)
                              { [self updateNode:self->m_actionSheetId]; }];
    [alertController addAction:action];
  }

  if (buttons & CancelDownloadAction)
  {
    UIAlertAction * action = [UIAlertAction actionWithTitle:kCancelDownloadActionTitle
                                                      style:UIAlertActionStyleDestructive
                                                    handler:^(UIAlertAction * action)
                              { [self cancelNode:self->m_actionSheetId]; }];
    [alertController addAction:action];
  }

  if (buttons & RetryDownloadAction)
  {
    UIAlertAction * action = [UIAlertAction actionWithTitle:kRetryActionTitle
                                                      style:UIAlertActionStyleDestructive
                                                    handler:^(UIAlertAction * action)
                              { [self retryDownloadNode:self->m_actionSheetId]; }];
    [alertController addAction:action];
  }

  if (buttons & DeleteAction)
  {
    UIAlertAction * action = [UIAlertAction actionWithTitle:kDeleteActionTitle
                                                      style:UIAlertActionStyleDestructive
                                                    handler:^(UIAlertAction * action)
                              { [self deleteNode:self->m_actionSheetId]; }];
    [alertController addAction:action];
  }

  UIAlertAction * action = [UIAlertAction actionWithTitle:kCancelActionTitle
                                                    style:UIAlertActionStyleCancel
                                                  handler:nil];
  [alertController addAction:action];
}

#pragma mark - Countries tree(s) navigation

- (void)openAvailableMaps
{
  BOOL const isParentRoot = [self.parentCountryId isEqualToString:@(GetFramework().GetStorage().GetRootId().c_str())];
  NSString * identifier = isParentRoot ? kControllerIdentifier : kBaseControllerIdentifier;
  MWMBaseMapDownloaderViewController * vc = [self.storyboard instantiateViewControllerWithIdentifier:identifier];
  [vc setParentCountryId:self.parentCountryId mode:MWMMapDownloaderModeAvailable];
  [MWMSegue segueFrom:self to:vc];
}

#pragma mark - MWMMapDownloaderProtocol

- (void)openNodeSubtree:(storage::TCountryId const &)countryId
{
  MWMBaseMapDownloaderViewController * vc = [self.storyboard instantiateViewControllerWithIdentifier:kBaseControllerIdentifier];
  [vc setParentCountryId:@(countryId.c_str()) mode:self.mode];
  [MWMSegue segueFrom:self to:vc];
}

- (void)downloadNode:(storage::TCountryId const &)countryId
{
  [Statistics logEvent:kStatDownloaderMapAction
        withParameters:@{
          kStatAction : kStatDownload,
          kStatIsAuto : kStatNo,
          kStatFrom : kStatDownloader,
          kStatScenario : kStatDownload
        }];
  self.skipCountryEventProcessing = YES;
  [MWMStorage downloadNode:countryId onSuccess:nil];
  self.skipCountryEventProcessing = NO;
  [self processCountryEvent:countryId];
}

- (void)retryDownloadNode:(storage::TCountryId const &)countryId
{
  [Statistics logEvent:kStatDownloaderMapAction
        withParameters:@{
          kStatAction : kStatRetry,
          kStatIsAuto : kStatNo,
          kStatFrom : kStatDownloader,
          kStatScenario : kStatDownload
        }];
  self.skipCountryEventProcessing = YES;
  [MWMStorage retryDownloadNode:countryId];
  self.skipCountryEventProcessing = NO;
  [self processCountryEvent:countryId];
}

- (void)updateNode:(storage::TCountryId const &)countryId
{
  [Statistics logEvent:kStatDownloaderMapAction
        withParameters:@{
          kStatAction : kStatUpdate,
          kStatIsAuto : kStatNo,
          kStatFrom : kStatDownloader,
          kStatScenario : kStatUpdate
        }];
  self.skipCountryEventProcessing = YES;
  [MWMStorage updateNode:countryId];
  self.skipCountryEventProcessing = NO;
  [self processCountryEvent:countryId];
}

- (void)deleteNode:(storage::TCountryId const &)countryId
{
  [Statistics logEvent:kStatDownloaderMapAction
        withParameters:@{
          kStatAction : kStatDelete,
          kStatIsAuto : kStatNo,
          kStatFrom : kStatDownloader,
          kStatScenario : kStatDelete
        }];
  self.skipCountryEventProcessing = YES;
  [MWMStorage deleteNode:countryId];
  self.skipCountryEventProcessing = NO;
  [self processCountryEvent:countryId];
}

- (void)cancelNode:(storage::TCountryId const &)countryId
{
  [Statistics logEvent:kStatDownloaderDownloadCancel withParameters:@{kStatFrom : kStatDownloader}];
  self.skipCountryEventProcessing = YES;
  [MWMStorage cancelDownloadNode:countryId];
  self.skipCountryEventProcessing = NO;
  [self processCountryEvent:countryId];
}

- (void)showNode:(storage::TCountryId const &)countryId
{
  [Statistics logEvent:kStatDownloaderMapAction
        withParameters:@{
          kStatAction : kStatExplore,
          kStatFrom : kStatDownloader,
        }];
  [self.navigationController popToRootViewControllerAnimated:YES];
  [MWMStorage showNode:countryId];
}

#pragma mark - Managing the Status Bar

- (UIStatusBarStyle)preferredStatusBarStyle
{
  if ([MWMToast affectsStatusBar])
    return [MWMToast preferredStatusBarStyle];
  setStatusBarBackgroundColor(UIColor.clearColor);
  return UIStatusBarStyleLightContent;
}

#pragma mark - Configuration

- (void)setParentCountryId:(NSString *)parentId mode:(MWMMapDownloaderMode)mode
{
  self.defaultDataSource = [[MWMMapDownloaderDefaultDataSource alloc] initForRootCountryId:parentId
                                                                                  delegate:self
                                                                                      mode:mode];
}

#pragma mark - Properties

- (void)setTableView:(UITableView *)tableView
{
  _tableView = tableView;
  tableView.tableFooterView = [[UIView alloc] initWithFrame:{}];
  self.dataSource = self.defaultDataSource;
}

- (void)setShowAllMapsButtons:(BOOL)showAllMapsButtons
{
  _showAllMapsButtons = showAllMapsButtons;
  self.allMapsView.hidden = !showAllMapsButtons;
}

- (NSString *)parentCountryId
{
  return self.dataSource.parentCountryId;
}
- (MWMMapDownloaderMode)mode { return self.dataSource.mode; }
- (void)setDataSource:(MWMMapDownloaderDataSource *)dataSource
{
  self.forceFullReload = YES;

  // Order matters. _dataSource must be set last since self.tableView does not retain dataSource.
  // In different order outdated datasource gets reclaimed between assignments.
  self.tableView.dataSource = dataSource;
  _dataSource = dataSource;
}

#pragma mark - Helpers

- (void)reloadData
{
  MWMMapDownloaderDefaultDataSource * defaultDataSource = self.defaultDataSource;
  [defaultDataSource load];
  if (self.dataSource == defaultDataSource)
    [self reloadTable];
}

- (void)reloadTable
{
  UITableView * tableView = self.tableView;
  // If these methods are not called, tableView will not call tableView:cellForRowAtIndexPath:
  [tableView setNeedsLayout];
  [tableView layoutIfNeeded];

  [tableView reloadData];

  // If these methods are not called, tableView will not display new cells
  [tableView setNeedsLayout];
  [tableView layoutIfNeeded];
}

@end
