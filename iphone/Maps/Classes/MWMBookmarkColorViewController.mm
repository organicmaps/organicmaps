#import "Common.h"
#import "MWMBookmarkColorCell.h"
#import "MWMBookmarkColorViewController.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManager.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"
#import "UIViewController+navigation.h"

namespace
{

NSArray<NSString *> * const kBookmarkColorsVariant = @[
  @"placemark-red",
  @"placemark-yellow",
  @"placemark-blue",
  @"placemark-green",
  @"placemark-purple",
  @"placemark-orange",
  @"placemark-brown",
  @"placemark-pink"
];

NSString * const kBookmarkColorCellIdentifier = @"MWMBookmarkColorCell";

} // namespace


@interface MWMBookmarkColorViewController ()

@property (nonatomic) BOOL colorWasChanged;

@end

@implementation MWMBookmarkColorViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"bookmark_color");
  [self.tableView registerNib:[UINib nibWithNibName:kBookmarkColorCellIdentifier bundle:nil] forCellReuseIdentifier:kBookmarkColorCellIdentifier];
  self.colorWasChanged = NO;
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  if (!self.iPadOwnerNavigationController)
    return;

  [self.iPadOwnerNavigationController setNavigationBarHidden:NO];
  [self showBackButton];
  CGFloat const navBarHeight = self.navigationController.navigationBar.height;
  [self.tableView reloadData];
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    [self.placePageManager changeHeight:self.tableView.contentSize.height + navBarHeight];
  }];
}

- (void)backTap
{
  if (self.iPadOwnerNavigationController)
   [self.iPadOwnerNavigationController setNavigationBarHidden:YES];
  [self.placePageManager reloadBookmark];
  [self.navigationController popViewControllerAnimated:YES];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [super viewWillDisappear:animated];
  if (self.colorWasChanged && !self.iPadOwnerNavigationController)
    [self.placePageManager reloadBookmark];
}

@end

@implementation MWMBookmarkColorViewController (TableView)

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  MWMBookmarkColorCell * cell = (MWMBookmarkColorCell *)[tableView dequeueReusableCellWithIdentifier:kBookmarkColorCellIdentifier];
  if (!cell)
    cell = [[MWMBookmarkColorCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:kBookmarkColorCellIdentifier];

  NSString * const currentColor = kBookmarkColorsVariant[indexPath.row];
  [cell configureWithColorString:kBookmarkColorsVariant[indexPath.row]];

  if ([currentColor isEqualToString:self.placePageManager.entity.bookmarkColor] && !cell.selected)
    [tableView selectRowAtIndexPath:indexPath animated:NO scrollPosition:UITableViewScrollPositionNone];

  return cell;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return kBookmarkColorsVariant.count;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSString * bookmarkColor = kBookmarkColorsVariant[indexPath.row];
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatChangeBookmarkColor)
                   withParameters:@{kStatValue : bookmarkColor}];
  self.colorWasChanged = YES;
  self.placePageManager.entity.bookmarkColor = bookmarkColor;
  if (!self.iPadOwnerNavigationController)
    return;

  [self.placePageManager.entity synchronize];
}

@end
