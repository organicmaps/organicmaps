#import "Common.h"
#import "MapsAppDelegate.h"
#import "MWMAlertViewController.h"
#import "MWMFrameworkListener.h"
#import "MWMMapDownloaderDefaultDataSource.h"
#import "MWMMapDownloaderLargeCountryTableViewCell.h"
#import "MWMMapDownloaderPlaceTableViewCell.h"
#import "MWMMapDownloaderSubplaceTableViewCell.h"
#import "MWMMapDownloaderTableViewCell.h"
#import "MWMMapDownloaderViewController.h"
#import "MWMSegue.h"
#import "MWMStorage.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"
#import "UIViewController+Navigation.h"

#include "Framework.h"

#include "storage/index.hpp"

extern NSString * const kCountryCellIdentifier = @"MWMMapDownloaderTableViewCell";
extern NSString * const kSubplaceCellIdentifier = @"MWMMapDownloaderSubplaceTableViewCell";
extern NSString * const kPlaceCellIdentifier = @"MWMMapDownloaderPlaceTableViewCell";
extern NSString * const kLargeCountryCellIdentifier = @"MWMMapDownloaderLargeCountryTableViewCell";
extern NSString * const kButtonCellIdentifier = @"MWMMapDownloaderButtonTableViewCell";

namespace
{
NSString * const kAllMapsLabelFormat = @"%@ %@ (%@)";
NSString * const kCancelActionTitle = L(@"cancel");
NSString * const kCancelAllTitle = L(@"downloader_cancel_all");
NSString * const kDeleteActionTitle = L(@"downloader_delete_map");
NSString * const kDownloaAllTitle = L(@"downloader_download_all_button");
NSString * const kDownloadActionTitle = L(@"downloader_download_map");
NSString * const kDownloadingTitle = L(@"downloader_downloading");
NSString * const kMapsTitle = L(@"downloader_maps");
NSString * const kShowActionTitle = L(@"zoom_to_country");
NSString * const kUpdateActionTitle = L(@"downloader_status_outdated");

NSString * const kBaseControllerIdentifier = @"MWMBaseMapDownloaderViewController";
NSString * const kControllerIdentifier = @"MWMMapDownloaderViewController";
} // namespace

@interface MWMBaseMapDownloaderViewController () <UIActionSheetDelegate, MWMFrameworkStorageObserver>

@property (weak, nonatomic) IBOutlet UITableView * tableView;

@property (weak, nonatomic) IBOutlet UIView * allMapsView;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * allMapsViewBottomOffset;
@property (weak, nonatomic) IBOutlet UIButton * allMapsButton;

@property (nonatomic) UIImage * navBarBackground;
@property (nonatomic) UIImage * navBarShadow;

@property (nonatomic) CGFloat lastScrollOffset;

@property (nonatomic) MWMMapDownloaderDataSource * dataSource;
@property (nonatomic) MWMMapDownloaderDefaultDataSource * defaultDataSource;

@property (nonatomic) NSMutableDictionary * offscreenCells;
@property (nonatomic) NSMutableDictionary<NSIndexPath *, NSNumber *> * cellHeightCache;

@property (nonatomic) BOOL skipCountryEventProcessing;
@property (nonatomic) BOOL forceFullReload;

@property (nonatomic, readonly) NSString * parentCountryId;
@property (nonatomic, readonly) TMWMMapDownloaderMode mode;

@end

using namespace storage;

@implementation MWMBaseMapDownloaderViewController
{
  TCountryId m_actionSheetId;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self configNavBar];
  [self configTable];
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
  BOOL const downloaded = self.mode == TMWMMapDownloaderMode::Downloaded;
  if (self.dataSource.isParentRoot)
  {
    self.title = downloaded ? L(@"downloader_my_maps_title") : L(@"download_maps");
  }
  else
  {
    NodeAttrs nodeAttrs;
    GetFramework().Storage().GetNodeAttrs(self.parentCountryId.UTF8String, nodeAttrs);
    self.title = @(nodeAttrs.m_nodeLocalName.c_str());
  }

  if (downloaded)
  {
    UIBarButtonItem * addButton =
        [self navBarButtonWithImage:[UIImage imageNamed:@"ic_nav_bar_add"]
                   highlightedImage:[UIImage imageNamed:@"ic_nav_bar_add_press"]
                             action:@selector(openAvailableMaps)];
    self.navigationItem.rightBarButtonItems = [self alignedNavBarButtonItems:@[ addButton ]];
  }
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
  [self.cellHeightCache removeAllObjects];
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
  auto const & s = GetFramework().Storage();
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

- (void)registerCellWithIdentifier:(NSString *)identifier
{
  [self.tableView registerNib:[UINib nibWithNibName:identifier bundle:nil] forCellReuseIdentifier:identifier];
}

- (void)configTable
{
  self.tableView.separatorColor = [UIColor blackDividers];
  if (isIOS7)
  {
    self.offscreenCells = [@{} mutableCopy];
    self.cellHeightCache = [@{} mutableCopy];
  }
  [self registerCellWithIdentifier:kPlaceCellIdentifier];
  [self registerCellWithIdentifier:kCountryCellIdentifier];
  [self registerCellWithIdentifier:kLargeCountryCellIdentifier];
  [self registerCellWithIdentifier:kSubplaceCellIdentifier];
  [self registerCellWithIdentifier:kButtonCellIdentifier];
}

- (void)showActionSheetForRowAtIndexPath:(NSIndexPath *)indexPath
{
  auto const & s = GetFramework().Storage();
  NodeAttrs nodeAttrs;
  m_actionSheetId = [self.dataSource countryIdForIndexPath:indexPath].UTF8String;
  s.GetNodeAttrs(m_actionSheetId, nodeAttrs);
  BOOL const needsUpdate = (nodeAttrs.m_status == NodeStatus::OnDiskOutOfDate);
  BOOL const isDownloaded = (needsUpdate || nodeAttrs.m_status == NodeStatus::OnDisk);
  NSString * title = @(nodeAttrs.m_nodeLocalName.c_str());
  BOOL const isMultiParent = (nodeAttrs.m_parentInfo.size() > 1);
  NSString * message = (self.dataSource.isParentRoot || isMultiParent)
                           ? nil
                           : @(nodeAttrs.m_parentInfo[0].m_localName.c_str());
  NSString * downloadActionTitle = [NSString
      stringWithFormat:@"%@, %@", kDownloadActionTitle, formattedSize(nodeAttrs.m_mwmSize)];
  if (isIOS7)
  {
    UITableViewCell * cell = [self.tableView cellForRowAtIndexPath:indexPath];
    UIView * cellSuperView = cell.superview;
    if (!cellSuperView)
      return;
    UIActionSheet * actionSheet = [[UIActionSheet alloc] initWithTitle:title
                                                              delegate:self
                                                     cancelButtonTitle:nil
                                                destructiveButtonTitle:nil
                                                     otherButtonTitles:nil];
    if (isDownloaded)
    {
      [actionSheet addButtonWithTitle:kShowActionTitle];
      if (needsUpdate)
        [actionSheet addButtonWithTitle:kUpdateActionTitle];
      [actionSheet addButtonWithTitle:kDeleteActionTitle];
      actionSheet.destructiveButtonIndex = actionSheet.numberOfButtons - 1;
    }
    else
    {
      [actionSheet addButtonWithTitle:downloadActionTitle];
    }
    if (!IPAD)
    {
      [actionSheet addButtonWithTitle:kCancelActionTitle];
      actionSheet.cancelButtonIndex = actionSheet.numberOfButtons - 1;
    }
    [actionSheet showFromRect:cell.frame inView:cellSuperView animated:YES];
  }
  else
  {
    UIAlertController * alertController =
        [UIAlertController alertControllerWithTitle:title
                                            message:message
                                     preferredStyle:UIAlertControllerStyleActionSheet];
    UITableViewCell * cell = [self.tableView cellForRowAtIndexPath:indexPath];
    alertController.popoverPresentationController.sourceView = cell;
    alertController.popoverPresentationController.sourceRect = cell.bounds;
    if (isDownloaded)
    {
      UIAlertAction * showAction = [UIAlertAction actionWithTitle:kShowActionTitle
                                                            style:UIAlertActionStyleDefault
                                                          handler:^(UIAlertAction * action)
                                    {
                                      [self showNode:self->m_actionSheetId];
                                    }];
      [alertController addAction:showAction];
      if (needsUpdate)
      {
        UIAlertAction * updateAction = [UIAlertAction actionWithTitle:kUpdateActionTitle
                                                                style:UIAlertActionStyleDefault
                                                              handler:^(UIAlertAction * action)
                                        {
                                          [self updateNode:self->m_actionSheetId];
                                        }];
        [alertController addAction:updateAction];
      }
      UIAlertAction * deleteAction = [UIAlertAction actionWithTitle:kDeleteActionTitle
                                                              style:UIAlertActionStyleDestructive
                                                            handler:^(UIAlertAction * action)
                                      {
                                        [self deleteNode:self->m_actionSheetId];
                                      }];
      [alertController addAction:deleteAction];
    }
    else
    {
      UIAlertAction * downloadAction = [UIAlertAction actionWithTitle:downloadActionTitle
                                                                style:UIAlertActionStyleDefault
                                                              handler:^(UIAlertAction * action)
                                        {
                                          [self downloadNode:self->m_actionSheetId];
                                        }];
      [alertController addAction:downloadAction];
    }
    UIAlertAction * cancelAction = [UIAlertAction actionWithTitle:kCancelActionTitle
                                                            style:UIAlertActionStyleCancel
                                                          handler:nil];
    [alertController addAction:cancelAction];

    [self presentViewController:alertController animated:YES completion:nil];
  }
}

#pragma mark - Offscreen cells

- (MWMMapDownloaderTableViewCell *)offscreenCellForIdentifier:(NSString *)reuseIdentifier
{
  MWMMapDownloaderTableViewCell * cell = self.offscreenCells[reuseIdentifier];
  if (!cell)
  {
    cell = [[[NSBundle mainBundle] loadNibNamed:reuseIdentifier owner:nil options:nil] firstObject];
    self.offscreenCells[reuseIdentifier] = cell;
  }
  return cell;
}

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
  self.showAllMapsView = YES;
  auto const & s = GetFramework().Storage();
  TCountriesVec downloadedChildren;
  TCountriesVec availableChildren;
  TCountryId const parentCountryId = self.parentCountryId.UTF8String;
  s.GetChildrenInGroups(parentCountryId, downloadedChildren, availableChildren);

  if (availableChildren.empty())
  {
    TCountriesVec queuedChildren;
    s.GetQueuedChildren(parentCountryId, queuedChildren);
    if (!queuedChildren.empty())
    {
      TMwmSize queuedSize = 0;
      for (TCountryId const & countryId : queuedChildren)
      {
        NodeAttrs nodeAttrs;
        s.GetNodeAttrs(countryId, nodeAttrs);
        queuedSize += nodeAttrs.m_mwmSize;
      }
      self.allMapsLabel.text =
          [NSString stringWithFormat:kAllMapsLabelFormat, kDownloadingTitle,
                                     @(queuedChildren.size()), formattedSize(queuedSize)];
      [self.allMapsButton setTitle:kCancelAllTitle forState:UIControlStateNormal];
      return;
    }
  }
  else
  {
    NodeAttrs nodeAttrs;
    s.GetNodeAttrs(parentCountryId, nodeAttrs);
    uint32_t remoteMWMCounter = nodeAttrs.m_mwmCounter - nodeAttrs.m_localMwmCounter;
    if (remoteMWMCounter != 0)
    {
      self.allMapsLabel.text =
          [NSString stringWithFormat:kAllMapsLabelFormat, kMapsTitle, @(remoteMWMCounter),
                                     formattedSize(nodeAttrs.m_mwmSize - nodeAttrs.m_localMwmSize)];
      [self.allMapsButton setTitle:kDownloaAllTitle forState:UIControlStateNormal];
      return;
    }
  }

  self.showAllMapsView = NO;
}

- (void)refreshAllMapsView
{
  [self refreshAllMapsViewForOffset:self.tableView.contentOffset.y];
}

- (void)refreshAllMapsViewForOffset:(CGFloat)scrollOffset
{
  if (!self.showAllMapsView)
    return;
  BOOL const hide = (scrollOffset >= self.lastScrollOffset) && !equalScreenDimensions(scrollOffset, 0.0);
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
  if (parentCountryId == GetFramework().Storage().GetRootId())
  {
    [Statistics logEvent:kStatDownloaderMapAction
          withParameters:@{
            kStatAction : kStatUpdate,
            kStatIsAuto : kStatNo,
            kStatFrom : kStatDownloader,
            kStatScenario : kStatUpdateAll
          }];
    [MWMStorage updateNode:parentCountryId alertController:self.alertController];
  }
  else
  {
    NSString * allMapsButtonTitle = [self.allMapsButton titleForState:UIControlStateNormal];
    if ([allMapsButtonTitle isEqualToString:kDownloaAllTitle])
    {
      [Statistics logEvent:kStatDownloaderMapAction
            withParameters:@{
              kStatAction : kStatDownload,
              kStatIsAuto : kStatNo,
              kStatFrom : kStatDownloader,
              kStatScenario : kStatDownloadGroup
            }];
      [MWMStorage downloadNode:parentCountryId
               alertController:self.alertController
                     onSuccess:nil];
    }
    else if ([allMapsButtonTitle isEqualToString:kCancelAllTitle])
    {
      [Statistics logEvent:kStatDownloaderMapAction
            withParameters:@{
              kStatAction : kStatCancel,
              kStatIsAuto : kStatNo,
              kStatFrom : kStatDownloader,
              kStatScenario : kStatDownloadGroup
            }];
      [MWMStorage cancelDownloadNode:parentCountryId];
    }
  }
  self.skipCountryEventProcessing = NO;
  [self processCountryEvent:parentCountryId];
}

#pragma mark - Countries tree(s) navigation

- (void)openAvailableMaps
{
  BOOL const isParentRoot = [self.parentCountryId isEqualToString:@(GetFramework().Storage().GetRootId().c_str())];
  NSString * identifier = isParentRoot ? kControllerIdentifier : kBaseControllerIdentifier;
  MWMBaseMapDownloaderViewController * vc = [self.storyboard instantiateViewControllerWithIdentifier:identifier];
  [vc setParentCountryId:self.parentCountryId mode:TMWMMapDownloaderMode::Available];
  [MWMSegue segueFrom:self to:vc];
}

- (void)openSubtreeForParentCountryId:(NSString *)parentCountryId
{
  MWMBaseMapDownloaderViewController * vc = [self.storyboard instantiateViewControllerWithIdentifier:kBaseControllerIdentifier];
  [vc setParentCountryId:parentCountryId mode:self.mode];
  [MWMSegue segueFrom:self to:vc];
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
  NSString * identifier = [self.dataSource cellIdentifierForIndexPath:indexPath];
  if ([identifier isEqualToString:kLargeCountryCellIdentifier])
    [self openSubtreeForParentCountryId:[self.dataSource countryIdForIndexPath:indexPath]];
  else
    [self showActionSheetForRowAtIndexPath:indexPath];
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (!isIOS7)
    return UITableViewAutomaticDimension;
  NSNumber * cacheHeight = self.cellHeightCache[indexPath];
  if (cacheHeight)
    return cacheHeight.floatValue;
  NSString * reuseIdentifier = [self.dataSource cellIdentifierForIndexPath:indexPath];
  MWMMapDownloaderTableViewCell * cell = [self offscreenCellForIdentifier:reuseIdentifier];
  [self.dataSource fillCell:cell atIndexPath:indexPath];
  cell.bounds = {{}, {CGRectGetWidth(tableView.bounds), CGRectGetHeight(cell.bounds)}};
  [cell setNeedsLayout];
  [cell layoutIfNeeded];
  CGSize const size = [cell.contentView systemLayoutSizeFittingSize:UILayoutFittingCompressedSize];
  CGFloat const height = ceil(size.height + 0.5);
  self.cellHeightCache[indexPath] = @(height);
  return height;
}

- (CGFloat)tableView:(UITableView *)tableView estimatedHeightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if ([self.dataSource isButtonCell:indexPath.section])
    return [MWMMapDownloaderButtonTableViewCell estimatedHeight];
  Class<MWMMapDownloaderTableViewCellProtocol> cellClass = NSClassFromString([self.dataSource cellIdentifierForIndexPath:indexPath]);
  return [cellClass estimatedHeight];
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
{
  if ([self.dataSource isButtonCell:section])
    return 36.0;
  return 28.0;
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section
{
  return 0.0;
}

#pragma mark - UILongPressGestureRecognizer

- (IBAction)longPress:(UILongPressGestureRecognizer *)sender
{
  if (sender.state != UIGestureRecognizerStateBegan)
    return;
  NSIndexPath * indexPath = [self.tableView indexPathForRowAtPoint:[sender locationInView:self.tableView]];
  if (indexPath && ![self.dataSource isButtonCell:indexPath.section])
    [self showActionSheetForRowAtIndexPath:indexPath];
}

#pragma mark - UIActionSheetDelegate

- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex
{
  if (buttonIndex >= actionSheet.numberOfButtons)
  {
    [actionSheet dismissWithClickedButtonIndex:0 animated:NO];
    return;
  }
  NSString * btnTitle = [actionSheet buttonTitleAtIndex:buttonIndex];
  if ([btnTitle hasPrefix:kDownloadActionTitle])
    [self downloadNode:m_actionSheetId];
  else if ([btnTitle isEqualToString:kDeleteActionTitle])
    [self deleteNode:m_actionSheetId];
  else if ([btnTitle isEqualToString:kUpdateActionTitle])
    [self updateNode:m_actionSheetId];
  else if ([btnTitle isEqualToString:kShowActionTitle])
    [self showNode:m_actionSheetId];
}

#pragma mark - MWMMapDownloaderProtocol

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
  [MWMStorage downloadNode:countryId alertController:self.alertController onSuccess:nil];
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
  [MWMStorage updateNode:countryId alertController:self.alertController];
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
  [MWMStorage deleteNode:countryId alertController:self.alertController];
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
  return UIStatusBarStyleLightContent;
}

#pragma mark - Configuration

- (void)setParentCountryId:(NSString *)parentId mode:(TMWMMapDownloaderMode)mode
{
  self.defaultDataSource = [[MWMMapDownloaderDefaultDataSource alloc] initForRootCountryId:parentId
                                                                                  delegate:self
                                                                                      mode:mode];
}

#pragma mark - Properties

- (void)setTableView:(UITableView *)tableView
{
  _tableView = tableView;
  _tableView.tableFooterView = [[UIView alloc] initWithFrame:{}];
  self.dataSource = self.defaultDataSource;
}

- (void)setShowAllMapsView:(BOOL)showAllMapsView
{
  _showAllMapsView = showAllMapsView;
  self.allMapsView.hidden = !showAllMapsView;
}

- (NSString *)parentCountryId
{
  return self.dataSource.parentCountryId;
}

- (TMWMMapDownloaderMode)mode
{
  return self.dataSource.mode;
}

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
  [self.cellHeightCache removeAllObjects];

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
