#import "Common.h"
#import "MWMBaseMapDownloaderViewController.h"
#import "MWMMapDownloaderCountryTableViewCell.h"
#import "MWMMapDownloaderLargeCountryTableViewCell.h"
#import "MWMMapDownloaderPlaceTableViewCell.h"
#import "MWMMapDownloaderSubplaceTableViewCell.h"
#import "MWMMapDownloaderTableViewHeader.h"
#import "MWMSegue.h"
#import "UIColor+MapsMeColor.h"

#include "Framework.h"

namespace
{
NSString * const kCountryCellIdentifier = @"MWMMapDownloaderCountryTableViewCell";
NSString * const kLargeCountryCellIdentifier = @"MWMMapDownloaderLargeCountryTableViewCell";
NSString * const kSubplaceCellIdentifier = @"MWMMapDownloaderSubplaceTableViewCell";
NSString * const kPlaceCellIdentifier = @"MWMMapDownloaderPlaceTableViewCell";
} // namespace


typedef void (^AlertActionType)(UIAlertAction *);

@interface MWMBaseMapDownloaderViewController () <UIActionSheetDelegate>

@property (weak, nonatomic) IBOutlet UITableView * tableView;

@property (weak, nonatomic) IBOutlet UIView * allMapsView;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * allMapsViewBottomOffset;

@property (nonatomic) UIImage * navBarBackground;
@property (nonatomic) UIImage * navBarShadow;

@property (nonatomic) CGFloat lastScrollOffset;

@property (copy, nonatomic) AlertActionType downloadAction;
@property (copy, nonatomic) AlertActionType showAction;

@property (copy, nonatomic) NSString * downloadActionTitle;
@property (copy, nonatomic) NSString * showActionTitle;
@property (copy, nonatomic) NSString * cancelActionTitle;

@property (nonatomic) NSArray<NSString *> * indexes;
@property (nonatomic) NSDictionary<NSString *, NSArray<NSString *> *> * countryIds;

@property (nonatomic, readonly) BOOL isParentRoot;

@end

using namespace storage;

@implementation MWMBaseMapDownloaderViewController
{
  TCountryId m_rootId;
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

#pragma mark - Data

- (void)configNavBar
{
  self.title = self.isParentRoot ? L(@"download_maps") : L(@([self GetRootCountryId].c_str()));
}

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
  self.offscreenCells = [NSMutableDictionary dictionary];
  [self registerCellWithIdentifier:kPlaceCellIdentifier];
  [self registerCellWithIdentifier:kCountryCellIdentifier];
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
  return self.isParentRoot ? kCountryCellIdentifier : kPlaceCellIdentifier;
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
// TODO (igrechuhin) Add implementation
  self.allMapsLabel.text = @"14 Maps (291 MB)";
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
  // TODO (igrechuhin) Add implementation
}

#pragma mark - Fill cells with data

- (void)fillCell:(MWMMapDownloaderTableViewCell * _Nonnull)cell atIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  auto const & s = GetFramework().Storage();
  NodeAttrs nodeAttrs;
  TCountryId countryId = [self countryIdForIndexPath:indexPath];
  s.GetNodeAttrs(countryId, nodeAttrs);
  [cell setTitleText:@(nodeAttrs.m_nodeLocalName.c_str())];
  [cell setDownloadSizeText:formattedSize(nodeAttrs.m_mwmSize)];
  [cell setLastCell:[self isLastRowForIndexPath:indexPath]];

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

#pragma mark - UITableViewDelegate

- (UIView * _Nullable)tableView:(UITableView * _Nonnull)tableView viewForHeaderInSection:(NSInteger)section
{
  if (!self.isParentRoot)
    return nil;
  MWMMapDownloaderTableViewHeader * headerView =
      [[[NSBundle mainBundle] loadNibNamed:@"MWMMapDownloaderTableViewHeader"
                                     owner:nil
                                   options:nil] firstObject];
  headerView.lastSection = (section == tableView.numberOfSections - 1);
  headerView.title.text = self.indexes[section];
  return headerView;
}

- (CGFloat)tableView:(UITableView * _Nonnull)tableView heightForHeaderInSection:(NSInteger)section
{
  return self.isParentRoot ? [MWMMapDownloaderTableViewHeader height] : 0.0;
}

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
// TODO (igrechuhin) Add implementation
//  NSString * title = @"Alessandria";
//  NSString * message = @"Italy, Piemont";
//  NSString * downloadActionTitle = [self.downloadActionTitle stringByAppendingString:@", 38 MB"];
//  if (isIOSVersionLessThan(8))
//  {
//    UIActionSheet * actionSheet = [[UIActionSheet alloc] initWithTitle:message
//                                                              delegate:self
//                                                     cancelButtonTitle:nil
//                                                destructiveButtonTitle:nil
//                                                     otherButtonTitles:nil];
//    [actionSheet addButtonWithTitle:downloadActionTitle];
//    [actionSheet addButtonWithTitle:self.showActionTitle];
//    if (!IPAD)
//    {
//      [actionSheet addButtonWithTitle:self.cancelActionTitle];
//      actionSheet.cancelButtonIndex = actionSheet.numberOfButtons - 1;
//    }
//    UITableViewCell * cell = [tableView cellForRowAtIndexPath:indexPath];
//    [actionSheet showFromRect:cell.frame inView:cell.superview animated:YES];
//  }
//  else
//  {
//    UIAlertController * alertController =
//        [UIAlertController alertControllerWithTitle:title
//                                            message:message
//                                     preferredStyle:UIAlertControllerStyleActionSheet];
//    UIAlertAction * downloadAction = [UIAlertAction actionWithTitle:downloadActionTitle
//                                                              style:UIAlertActionStyleDefault
//                                                            handler:self.downloadAction];
//    UIAlertAction * showAction = [UIAlertAction actionWithTitle:self.showActionTitle
//                                                          style:UIAlertActionStyleDefault
//                                                        handler:self.showAction];
//    UIAlertAction * cancelAction = [UIAlertAction actionWithTitle:self.cancelActionTitle
//                                                            style:UIAlertActionStyleCancel
//                                                          handler:nil];
//    [alertController addAction:downloadAction];
//    [alertController addAction:showAction];
//    [alertController addAction:cancelAction];
//    [self presentViewController:alertController animated:YES completion:nil];
//  }
  }
}

- (CGFloat)tableView:(UITableView * _Nonnull)tableView heightForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  NSString * reuseIdentifier = [self cellIdentifierForIndexPath:indexPath];
  MWMMapDownloaderTableViewCell * cell = [self offscreenCellForIdentifier:reuseIdentifier];
  [self fillCell:cell atIndexPath:indexPath];
  [cell setNeedsUpdateConstraints];
  [cell updateConstraintsIfNeeded];
  cell.bounds = {{}, {CGRectGetWidth(tableView.bounds), CGRectGetHeight(cell.bounds)}};
  [cell setNeedsLayout];
  [cell layoutIfNeeded];
  CGSize const size = [cell.contentView systemLayoutSizeFittingSize:UILayoutFittingCompressedSize];
  return size.height;
}

- (CGFloat)tableView:(UITableView * _Nonnull)tableView estimatedHeightForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  NSString * reuseIdentifier = [self cellIdentifierForIndexPath:indexPath];
  MWMMapDownloaderTableViewCell * cell = [self offscreenCellForIdentifier:reuseIdentifier];
  return cell.estimatedHeight;
}

- (void)tableView:(UITableView * _Nonnull)tableView willDisplayCell:(MWMMapDownloaderTableViewCell * _Nonnull)cell forRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  [self fillCell:cell atIndexPath:indexPath];
}

#pragma mark - UIActionSheetDelegate

- (void)actionSheet:(UIActionSheet * _Nonnull)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex
{
  NSString * btnTitle = [actionSheet buttonTitleAtIndex:buttonIndex];
  if ([btnTitle hasPrefix:self.downloadActionTitle])
    self.downloadAction(nil);
  else if ([btnTitle isEqualToString:self.showActionTitle])
    self.showAction(nil);
}

#pragma mark - Action Sheet actions

- (void)configActions
{
  self.downloadAction = ^(UIAlertAction * action)
  {
    // TODO (igrechuhin) Add implementation
  };

  self.showAction = ^(UIAlertAction * action)
  {
    // TODO (igrechuhin) Add implementation
  };

  self.downloadActionTitle = L(@"downloader_download_map");
  self.showActionTitle = L(@"zoom_to_country");
  self.cancelActionTitle = L(@"cancel");
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
