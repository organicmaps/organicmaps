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

#include "Framework.h"

#include "storage/index.hpp"

extern NSString * const kCountryCellIdentifier = @"MWMMapDownloaderTableViewCell";
extern NSString * const kSubplaceCellIdentifier = @"MWMMapDownloaderSubplaceTableViewCell";
extern NSString * const kPlaceCellIdentifier = @"MWMMapDownloaderPlaceTableViewCell";
extern NSString * const kLargeCountryCellIdentifier = @"MWMMapDownloaderLargeCountryTableViewCell";

namespace
{
NSString * const kDownloadActionTitle = L(@"downloader_download_map");
NSString * const kUpdateActionTitle = L(@"downloader_status_outdated");
NSString * const kDeleteActionTitle = L(@"downloader_delete_map");
NSString * const kShowActionTitle = L(@"zoom_to_country");
NSString * const kCancelActionTitle = L(@"cancel");
} // namespace

@interface MWMBaseMapDownloaderViewController () <UIActionSheetDelegate, MWMFrameworkStorageObserver>

@property (weak, nonatomic) IBOutlet UITableView * tableView;

@property (weak, nonatomic) IBOutlet UIView * allMapsView;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * allMapsViewBottomOffset;

@property (nonatomic) UIImage * navBarBackground;
@property (nonatomic) UIImage * navBarShadow;

@property (nonatomic) CGFloat lastScrollOffset;

@property (nonatomic) MWMMapDownloaderDataSource * dataSource;
@property (nonatomic) MWMMapDownloaderDefaultDataSource * defaultDataSource;

@property (nonatomic) NSMutableDictionary * offscreenCells;
@property (nonatomic) NSMutableDictionary<NSIndexPath *, NSNumber *> * cellHeightCache;

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
  [self configAllMapsView];
  [MWMFrameworkListener addObserver:self];
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
}

- (void)viewWillDisappear:(BOOL)animated
{
  [super viewWillDisappear:animated];
  UINavigationBar * navBar = [UINavigationBar appearance];
  [navBar setBackgroundImage:self.navBarBackground forBarMetrics:UIBarMetricsDefault];
  navBar.shadowImage = self.navBarShadow;
}

- (void)configNavBar
{
  self.title = self.dataSource.isParentRoot ? L(@"download_maps") : L(@(self.parentCountryId.c_str()));
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
  [self.cellHeightCache removeAllObjects];
}

#pragma mark - MWMFrameworkStorageObserver

- (void)processCountryEvent:(TCountryId const &)countryId
{
  auto process = ^
  {
    [self configAllMapsView];
    MWMMapDownloaderDefaultDataSource * dataSource = self.defaultDataSource;
    [dataSource reload];
    if (![self.dataSource isEqual:dataSource])
      return;

    if (dataSource.needFullReload)
    {
      [self reloadData];
      return;
    }

    UITableView * tv = self.tableView;
    std::vector<NSInteger> sections = [dataSource getReloadSections];
    if (sections.empty())
      return;
    NSMutableIndexSet * indexSet = [NSMutableIndexSet indexSet];
    for (auto & section : sections)
      [indexSet addIndex:section];
    [tv reloadSections:indexSet withRowAnimation:UITableViewRowAnimationAutomatic];
  };

  if (countryId == self.parentCountryId)
  {
    process();
  }
  else
  {
    TCountriesVec childrenId;
    GetFramework().Storage().GetChildren(self.parentCountryId, childrenId);
    if (find(childrenId.cbegin(), childrenId.cend(), countryId) != childrenId.cend())
      process();
  }

  for (MWMMapDownloaderTableViewCell * cell in self.tableView.visibleCells)
    [cell processCountryEvent:countryId];
}

- (void)processCountry:(TCountryId const &)countryId progress:(TLocalAndRemoteSize const &)progress
{
  for (MWMMapDownloaderTableViewCell * cell in self.tableView.visibleCells)
    [cell processCountry:countryId progress:progress];
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
}

- (void)showActionSheetForRowAtIndexPath:(NSIndexPath *)indexPath
{
  auto const & s = GetFramework().Storage();
  NodeAttrs nodeAttrs;
  m_actionSheetId = [self.dataSource countryIdForIndexPath:indexPath];
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
    UITableViewCell * cell = [self.tableView cellForRowAtIndexPath:indexPath];
    [actionSheet showFromRect:cell.frame inView:cell.superview animated:YES];
  }
  else
  {
    UIAlertController * alertController =
    [UIAlertController alertControllerWithTitle:title
                                        message:message
                                 preferredStyle:UIAlertControllerStyleActionSheet];
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
  if (self.dataSource.isParentRoot)
    return;
  auto const & s = GetFramework().Storage();
  NodeAttrs nodeAttrs;
  s.GetNodeAttrs(self.parentCountryId, nodeAttrs);
  uint32_t remoteMWMCounter = nodeAttrs.m_mwmCounter - nodeAttrs.m_localMwmCounter;
  if (remoteMWMCounter != 0)
  {
    self.showAllMapsView = YES;
    self.allMapsLabel.text =
        [NSString stringWithFormat:@"%@: %@ (%@)", L(@"maps"), @(remoteMWMCounter),
                                   formattedSize(nodeAttrs.m_mwmSize - nodeAttrs.m_localMwmSize)];
  }
  else
  {
    self.showAllMapsView = NO;
  }
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
  if (self.parentCountryId == GetFramework().Storage().GetRootId())
  {
    [Statistics logEvent:kStatDownloaderMapAction
          withParameters:@{
            kStatAction : kStatUpdate,
            kStatIsAuto : kStatNo,
            kStatFrom : kStatDownloader,
            kStatScenario : kStatUpdateAll
          }];
    [MWMStorage updateNode:self.parentCountryId alertController:self.alertController];
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
    [MWMStorage downloadNode:self.parentCountryId
             alertController:self.alertController
                   onSuccess:nil];
  }
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
  NSString * identifier = [self.dataSource cellIdentifierForIndexPath:indexPath];
  if ([identifier isEqualToString:kLargeCountryCellIdentifier])
  {
    MWMBaseMapDownloaderViewController * vc = [self.storyboard instantiateViewControllerWithIdentifier:@"MWMBaseMapDownloaderViewController"];
    vc.parentCountryId = [self.dataSource countryIdForIndexPath:indexPath];
    [MWMSegue segueFrom:self to:vc];
  }
  else
  {
    [self showActionSheetForRowAtIndexPath:indexPath];
  }
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
  Class<MWMMapDownloaderTableViewCellProtocol> cellClass = NSClassFromString([self.dataSource cellIdentifierForIndexPath:indexPath]);
  return [cellClass estimatedHeight];
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
{
  return 28.0;
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section
{
  return 0.0;
}

#pragma mark - UIActionSheetDelegate

- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex
{
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
  [MWMStorage downloadNode:countryId alertController:self.alertController onSuccess:nil];
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
  [MWMStorage retryDownloadNode:countryId];
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
  [MWMStorage updateNode:countryId alertController:self.alertController];
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
  [MWMStorage deleteNode:countryId];
}

- (void)cancelNode:(storage::TCountryId const &)countryId
{
  [Statistics logEvent:kStatDownloaderDownloadCancel withParameters:@{kStatFrom : kStatDownloader}];
  [MWMStorage cancelDownloadNode:countryId];
}

- (void)showNode:(storage::TCountryId const &)countryId
{
  [MapsAppDelegate showNode:countryId];
  [self.navigationController popToRootViewControllerAnimated:YES];
}

#pragma mark - Managing the Status Bar

- (UIStatusBarStyle)preferredStatusBarStyle
{
  return UIStatusBarStyleLightContent;
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

- (TCountryId)parentCountryId
{
  return [self.dataSource parentCountryId];
}

- (void)setParentCountryId:(TCountryId)parentId
{
  self.defaultDataSource = [[MWMMapDownloaderDefaultDataSource alloc] initForRootCountryId:parentId delegate:self];
}

- (void)setDataSource:(MWMMapDownloaderDataSource *)dataSource
{
  self.tableView.dataSource = dataSource;
}

- (MWMMapDownloaderDataSource *)dataSource
{
  return static_cast<MWMMapDownloaderDataSource *>(self.tableView.dataSource);
}

#pragma mark - Helpers

- (void)reloadData
{
  [self.cellHeightCache removeAllObjects];
  UITableView * tv = self.tableView;
  // If these methods are not called, tableView will not call tableView:cellForRowAtIndexPath:
  [tv setNeedsLayout];
  [tv layoutIfNeeded];

  [tv reloadData];

  // If these methods are not called, tableView will not display new cells
  [tv setNeedsLayout];
  [tv layoutIfNeeded];
}

@end
