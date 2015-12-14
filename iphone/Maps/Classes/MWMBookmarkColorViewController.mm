#import "MWMBookmarkColorCell.h"
#import "MWMBookmarkColorViewController.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManager.h"
#import "Statistics.h"
#import "UIViewController+navigation.h"

extern NSArray * const kBookmarkColorsVariant;

static NSString * const kBookmarkColorCellIdentifier = @"MWMBookmarkColorCell";

@interface MWMBookmarkColorViewController ()

@property (weak, nonatomic) IBOutlet UITableView * tableView;
@property (nonatomic) BOOL colorWasChanged;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * tableViewHeight;

@end

@interface MWMBookmarkColorViewController (TableView) <UITableViewDataSource, UITableViewDelegate>
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
  [self configureTableViewForOrientation:self.interfaceOrientation];
  [self.tableView reloadData];
  if (!self.iPadOwnerNavigationController)
    return;

  [self.iPadOwnerNavigationController setNavigationBarHidden:NO];
  CGFloat const bottomOffset = 88.;
  self.iPadOwnerNavigationController.view.height = self.tableView.height + bottomOffset;
  [self showBackButton];
}

- (void)backTap
{
  if (self.iPadOwnerNavigationController)
   [self.iPadOwnerNavigationController setNavigationBarHidden:YES];
  [self.placePageManager reloadBookmark];
  [self.navigationController popViewControllerAnimated:YES];
}

- (void)configureTableViewForOrientation:(UIInterfaceOrientation)orientation
{
  if (self.iPadOwnerNavigationController)
    return;

  CGSize const size = self.navigationController.view.bounds.size;
  CGFloat const defaultHeight = 352.;
  switch (orientation)
  {
    case UIInterfaceOrientationPortrait:
    case UIInterfaceOrientationPortraitUpsideDown:
    case UIInterfaceOrientationUnknown:
      self.tableViewHeight.constant = defaultHeight;
      break;
    case UIInterfaceOrientationLandscapeLeft:
    case UIInterfaceOrientationLandscapeRight:
    {
      CGFloat const topOffset = 24.;
      CGFloat const navBarHeight = 64.;
      self.tableViewHeight.constant = MIN(defaultHeight, MIN(size.width, size.height) - 2 * topOffset - navBarHeight);
      break;
    }
  }
  [self.tableView setNeedsLayout];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
  [self configureTableViewForOrientation:self.interfaceOrientation];
}

- (BOOL)shouldAutorotate
{
  return YES;
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
  [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatChangeBookmarkColor)
                   withParameters:@{kStatValue : bookmarkColor}];
  self.colorWasChanged = YES;
  self.placePageManager.entity.bookmarkColor = bookmarkColor;
  if (!self.iPadOwnerNavigationController)
    return;

  [self.placePageManager.entity synchronize];
}

@end
