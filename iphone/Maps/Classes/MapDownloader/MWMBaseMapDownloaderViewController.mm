#import "Common.h"
#import "MWMBaseMapDownloaderViewController.h"
#import "MWMMapDownloaderTableViewCell.h"
#import "MWMMapDownloaderLargeCountryTableViewCell.h"
#import "MWMMapDownloaderPlaceTableViewCell.h"
#import "MWMMapDownloaderSubplaceTableViewCell.h"
#import "MWMSegue.h"
#import "UIColor+MapsMeColor.h"

#include "Framework.h"

namespace
{
NSString * const kCellIdentifier = @"MWMMapDownloaderTableViewCell";
NSString * const kLargeCountryCellIdentifier = @"MWMMapDownloaderLargeCountryTableViewCell";
NSString * const kSubplaceCellIdentifier = @"MWMMapDownloaderSubplaceTableViewCell";
NSString * const kPlaceCellIdentifier = @"MWMMapDownloaderPlaceTableViewCell";

NSString * const kDownloadActionTitle = L(@"downloader_download_map");
NSString * const kDeleteActionTitle = L(@"downloader_delete_map");
NSString * const kShowActionTitle = L(@"zoom_to_country");
NSString * const kCancelActionTitle = L(@"cancel");

using TAlertAction = void (^)(UIAlertAction *);
} // namespace

@interface MWMBaseMapDownloaderViewController () <UIActionSheetDelegate>

@property (weak, nonatomic) IBOutlet UITableView * tableView;

@property (weak, nonatomic) IBOutlet UIView * allMapsView;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * allMapsViewBottomOffset;

@property (nonatomic) UIImage * navBarBackground;
@property (nonatomic) UIImage * navBarShadow;

@property (nonatomic) CGFloat lastScrollOffset;

@property (copy, nonatomic) TAlertAction downloadAction;
@property (copy, nonatomic) TAlertAction deleteAction;
@property (copy, nonatomic) TAlertAction showAction;

@property (nonatomic) NSArray<NSString *> * indexes;
@property (nonatomic) NSDictionary<NSString *, NSArray<NSString *> *> * countryIds;

@property (nonatomic, readonly) BOOL isParentRoot;

@end

using namespace storage;

@implementation MWMBaseMapDownloaderViewController
{
  TCountryId m_rootId;
  TCountryId m_actionSheetId;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self configNavBar];
  [self configTable];
  [self configAllMapsView];
  [self configActions];
  [self configData];
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
  navBar.shadowImage = [UIImage imageWithColor:[UIColor clearColor]];
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
  self.title = self.isParentRoot ? L(@"download_maps") : L(@([self GetRootCountryId].c_str()));
}

#pragma mark - Data

- (void)configData
{
  auto const & s = GetFramework().Storage();
  TCountriesVec children;
  s.GetChildren([self GetRootCountryId], children);
  NSMutableSet<NSString *> * indexSet = [NSMutableSet setWithCapacity:children.size()];
  NSMutableDictionary<NSString *, NSArray<NSString *> *> * countryIds = [@{} mutableCopy];
  for (auto const & countryId : children)
  {
    NSString * nsCountryId = @(countryId.c_str());
    NSString * localizedName = L(nsCountryId).capitalizedString;
    NSString * firstLetter = [localizedName substringToIndex:1];
    [indexSet addObject:firstLetter];

    NSMutableArray<NSString *> * letterIds = [countryIds[firstLetter] mutableCopy];
    letterIds = letterIds ? letterIds : [@[] mutableCopy];
    [letterIds addObject:nsCountryId];
    countryIds[firstLetter] = [letterIds copy];
  }
  auto sort = ^NSComparisonResult(NSString * s1, NSString * s2)
  {
    NSString * l1 = L(s1);
    return [l1 compare:L(s2) options:NSCaseInsensitiveSearch range:{0, l1.length} locale:[NSLocale currentLocale]];
  };
  self.indexes = [[indexSet allObjects] sortedArrayUsingComparator:sort];
  [countryIds enumerateKeysAndObjectsUsingBlock:^(NSString * key, NSArray<NSString *> * obj, BOOL * stop)
  {
    countryIds[key] = [obj sortedArrayUsingComparator:sort];
  }];
  self.countryIds = countryIds;
}

- (TCountryId)countryIdForIndexPath:(NSIndexPath *)indexPath
{
  NSString * firstLetter = self.indexes[indexPath.section];
  NSArray<NSString *> * countryIds = self.countryIds[firstLetter];
  NSString * nsCountryId = countryIds[indexPath.row];
  return nsCountryId.UTF8String;
}

- (BOOL)isLastRowForIndexPath:(NSIndexPath *)indexPath
{
  NSString * firstLetter = self.indexes[indexPath.section];
  NSArray<NSString *> * countryIds = self.countryIds[firstLetter];
  BOOL const isLastSection = self.indexes.count - 1 == indexPath.section;
  BOOL const isLastRow = countryIds.count - 1 == indexPath.row;
  return self.isParentRoot ? isLastRow : isLastSection && isLastRow;
}

#pragma mark - Table

- (void)registerCellWithIdentifier:(NSString *)identifier
{
  [self.tableView registerNib:[UINib nibWithNibName:identifier bundle:nil] forCellReuseIdentifier:identifier];
}

- (void)configTable
{
  self.tableView.separatorColor = [UIColor blackDividers];
  self.offscreenCells = [NSMutableDictionary dictionary];
  [self registerCellWithIdentifier:kPlaceCellIdentifier];
  [self registerCellWithIdentifier:kCellIdentifier];
  [self registerCellWithIdentifier:kLargeCountryCellIdentifier];
  [self registerCellWithIdentifier:kSubplaceCellIdentifier];
}

- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath
{
  auto const & s = GetFramework().Storage();
  TCountriesVec children;
  s.GetChildren([self countryIdForIndexPath:indexPath], children);
  BOOL const haveChildren = !children.empty();
  if (haveChildren)
    return kLargeCountryCellIdentifier;
  return self.isParentRoot ? kCellIdentifier : kPlaceCellIdentifier;
}

- (void)showActionSheetForRowAtIndexPath:(NSIndexPath *)indexPath
{
  auto const & s = GetFramework().Storage();
  NodeAttrs nodeAttrs;
  m_actionSheetId = [self countryIdForIndexPath:indexPath];
  s.GetNodeAttrs(m_actionSheetId, nodeAttrs);
  BOOL const isDownloaded = (nodeAttrs.m_status == NodeStatus::OnDisk ||
                             nodeAttrs.m_status == NodeStatus::OnDiskOutOfDate);
  NSString * title = @(nodeAttrs.m_nodeLocalName.c_str());
  NSString * message = self.isParentRoot ? nil : @(nodeAttrs.m_parentLocalName.c_str());
  NSString * downloadActionTitle = [NSString stringWithFormat:@"%@, %@", kDownloadActionTitle, formattedSize(nodeAttrs.m_mwmSize)];
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
                                                          handler:self.showAction];
      [alertController addAction:showAction];
      UIAlertAction * deleteAction = [UIAlertAction actionWithTitle:kDeleteActionTitle
                                                              style:UIAlertActionStyleDestructive
                                                            handler:self.deleteAction];
      [alertController addAction:deleteAction];
    }
    else
    {
      UIAlertAction * downloadAction = [UIAlertAction actionWithTitle:downloadActionTitle
                                                                style:UIAlertActionStyleDefault
                                                              handler:self.downloadAction];
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

- (void)scrollViewWillEndDragging:(UIScrollView * _Nonnull)scrollView withVelocity:(CGPoint)velocity targetContentOffset:(inout CGPoint * _Nonnull)targetContentOffset
{
  SEL const selector = @selector(refreshAllMapsView);
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:selector object:nil];
  [self refreshAllMapsViewForOffset:targetContentOffset->y];
}

- (void)scrollViewDidScroll:(UIScrollView * _Nonnull)scrollView
{
  SEL const selector = @selector(refreshAllMapsView);
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:selector object:nil];
  [self performSelector:selector withObject:nil afterDelay:kDefaultAnimationDuration];
}

#pragma mark - All Maps Action

- (void)configAllMapsView
{
  if (self.isParentRoot)
    return;
  auto const & s = GetFramework().Storage();
  NodeAttrs nodeAttrs;
  s.GetNodeAttrs(m_rootId, nodeAttrs);
  self.allMapsLabel.text =
      [NSString stringWithFormat:@"%@ %@ (%@)", L(@"maps"),
                                 @(nodeAttrs.m_mwmCounter - nodeAttrs.m_localMwmCounter),
                                 formattedSize(nodeAttrs.m_mwmSize - nodeAttrs.m_localMwmSize)];
  self.showAllMapsView = YES;
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
  GetFramework().Storage().DownloadNode(m_rootId);
}

#pragma mark - Fill cells with data

- (void)fillCell:(MWMMapDownloaderTableViewCell *)cell forCountryId:(TCountryId const &)countryId
{
  auto const & s = GetFramework().Storage();
  NodeAttrs nodeAttrs;
  s.GetNodeAttrs(countryId, nodeAttrs);
  [cell setTitleText:@(nodeAttrs.m_nodeLocalName.c_str())];
  [cell setDownloadSizeText:formattedSize(nodeAttrs.m_mwmSize)];

  if ([cell isKindOfClass:[MWMMapDownloaderLargeCountryTableViewCell class]])
  {
    MWMMapDownloaderLargeCountryTableViewCell * tCell = (MWMMapDownloaderLargeCountryTableViewCell *)cell;
    [tCell setMapCountText:@(nodeAttrs.m_mwmCounter).stringValue];
  }
  else if ([cell isKindOfClass:[MWMMapDownloaderPlaceTableViewCell class]])
  {
    MWMMapDownloaderPlaceTableViewCell * tCell = (MWMMapDownloaderPlaceTableViewCell *)cell;
    NSString * areaText = self.isParentRoot ? @(nodeAttrs.m_parentLocalName.c_str()) : @"";
    [tCell setAreaText:areaText];
  }
  else if ([cell isKindOfClass:[MWMMapDownloaderSubplaceTableViewCell class]])
  {
// TODO (igrechuhin) Add implementation
//    MWMMapDownloaderSubplaceTableViewCell * tCell = (MWMMapDownloaderSubplaceTableViewCell *)cell;
//    tCell.title.text = data[kCellTitle];
//    tCell.downloadSize.text = data[kCellDownloadSize];
//    tCell.area.text = data[kCellArea];
//    tCell.subPlace.text = data[kCellSubplace];
//    tCell.separator.hidden = isLastCellInSection;
  }
}

#pragma mark - UITableViewDataSource

- (UITableViewCell * _Nonnull)tableView:(UITableView * _Nonnull)tableView cellForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  NSString * reuseIdentifier = [self cellIdentifierForIndexPath:indexPath];
  return [tableView dequeueReusableCellWithIdentifier:reuseIdentifier];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView * _Nonnull)tableView
{
  return self.indexes.count;
}

- (NSInteger)tableView:(UITableView * _Nonnull)tableView numberOfRowsInSection:(NSInteger)section
{
  NSString * firstLetter = self.indexes[section];
  return self.countryIds[firstLetter].count;
}

- (NSArray<NSString *> * _Nullable)sectionIndexTitlesForTableView:(UITableView * _Nonnull)tableView
{
  return self.isParentRoot ? self.indexes : nil;
}

- (NSInteger)tableView:(UITableView * _Nonnull)tableView sectionForSectionIndexTitle:(NSString * _Nonnull)title atIndex:(NSInteger)index
{
  return index;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  return self.isParentRoot ? self.indexes[section] : nil;
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView * _Nonnull)tableView didSelectRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
  NSString * identifier = [self cellIdentifierForIndexPath:indexPath];
  if ([identifier isEqualToString:kLargeCountryCellIdentifier])
  {
    MWMBaseMapDownloaderViewController * vc = [self.storyboard instantiateViewControllerWithIdentifier:@"MWMBaseMapDownloaderViewController"];
    [vc SetRootCountryId:[self countryIdForIndexPath:indexPath]];
    [MWMSegue segueFrom:self to:vc];
  }
  else
  {
    [self showActionSheetForRowAtIndexPath:indexPath];
  }
}

- (CGFloat)tableView:(UITableView * _Nonnull)tableView heightForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  NSString * reuseIdentifier = [self cellIdentifierForIndexPath:indexPath];
  MWMMapDownloaderTableViewCell * cell = [self offscreenCellForIdentifier:reuseIdentifier];
  TCountryId countryId = [self countryIdForIndexPath:indexPath];
  [self fillCell:cell forCountryId:countryId];
  [cell setNeedsUpdateConstraints];
  [cell updateConstraintsIfNeeded];
  cell.bounds = {{}, {CGRectGetWidth(tableView.bounds), CGRectGetHeight(cell.bounds)}};
  [cell setNeedsLayout];
  [cell layoutIfNeeded];
  CGSize const size = [cell.contentView systemLayoutSizeFittingSize:UILayoutFittingCompressedSize];
  return ceil(size.height + 0.5);
}

- (CGFloat)tableView:(UITableView * _Nonnull)tableView estimatedHeightForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  NSString * reuseIdentifier = [self cellIdentifierForIndexPath:indexPath];
  MWMMapDownloaderTableViewCell * cell = [self offscreenCellForIdentifier:reuseIdentifier];
  return cell.estimatedHeight;
}

- (void)tableView:(UITableView * _Nonnull)tableView willDisplayCell:(MWMMapDownloaderTableViewCell * _Nonnull)cell forRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  [self fillCell:cell forCountryId:[self countryIdForIndexPath:indexPath]];
}

#pragma mark - UIActionSheetDelegate

- (void)actionSheet:(UIActionSheet * _Nonnull)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex
{
  NSString * btnTitle = [actionSheet buttonTitleAtIndex:buttonIndex];
  if ([btnTitle hasPrefix:kDownloadActionTitle])
    self.downloadAction(nil);
  else if ([btnTitle isEqualToString:kDeleteActionTitle])
    self.deleteAction(nil);
  else if ([btnTitle isEqualToString:kShowActionTitle])
    self.showAction(nil);
}

#pragma mark - Action Sheet actions

- (void)configActions
{
  __weak auto weakSelf = self;
  self.downloadAction = ^(UIAlertAction * action)
  {
    __strong auto self = weakSelf;
    GetFramework().Storage().DownloadNode(self->m_actionSheetId);
  };

  self.deleteAction = ^(UIAlertAction * action)
  {
    __strong auto self = weakSelf;
    GetFramework().Storage().DeleteNode(self->m_actionSheetId);
  };

  self.showAction = ^(UIAlertAction * action)
  {
    // TODO (igrechuhin) Add implementation
  };
}

#pragma mark - Managing the Status Bar

- (UIStatusBarStyle)preferredStatusBarStyle
{
  return UIStatusBarStyleLightContent;
}

#pragma mark - Properties

- (void)setShowAllMapsView:(BOOL)showAllMapsView
{
  _showAllMapsView = showAllMapsView;
  self.allMapsView.hidden = !showAllMapsView;
}

- (TCountryId)GetRootCountryId
{
  return m_rootId;
}

- (void)SetRootCountryId:(TCountryId)rootId
{
  m_rootId = rootId;
  _isParentRoot = (m_rootId == GetFramework().Storage().GetRootId());
}


@end
