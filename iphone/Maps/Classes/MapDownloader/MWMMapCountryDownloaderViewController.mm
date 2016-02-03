#import "Common.h"
#import "MWMMapCountryDownloaderViewController.h"
#import "UIColor+MapsMeColor.h"

extern NSString * const kPlaceCellIdentifier = @"MWMMapDownloaderPlaceTableViewCell";

typedef void (^AlertActionType)(UIAlertAction *);

@interface MWMMapCountryDownloaderViewController () <UIActionSheetDelegate>

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

@end

@implementation MWMMapCountryDownloaderViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self configTable];
  [self configAllMapsView];
  [self configActions];
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

#pragma mark - Table

- (void)configTable
{
  self.offscreenCells = [NSMutableDictionary dictionary];
  [self.tableView registerNib:[UINib nibWithNibName:kPlaceCellIdentifier bundle:nil] forCellReuseIdentifier:kPlaceCellIdentifier];
}

- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath
{
  return kPlaceCellIdentifier;
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
}

#pragma mark - Fill cells with data

- (void)fillCell:(MWMMapDownloaderTableViewCell * _Nonnull)cell atIndexPath:(NSIndexPath * _Nonnull)indexPath
{
}

#pragma mark - UITableViewDataSource

- (UITableViewCell * _Nonnull)tableView:(UITableView * _Nonnull)tableView cellForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  NSString * reuseIdentifier = [self cellIdentifierForIndexPath:indexPath];
  return [tableView dequeueReusableCellWithIdentifier:reuseIdentifier];
}

- (NSInteger)tableView:(UITableView * _Nonnull)tableView numberOfRowsInSection:(NSInteger)section
{
  return 20;
}

#pragma mark - UITableViewDelegate

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

- (void)tableView:(UITableView * _Nonnull)tableView didSelectRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
  NSString * title = @"Alessandria";
  NSString * message = @"Italy, Piemont";
  NSString * downloadActionTitle = [self.downloadActionTitle stringByAppendingString:@", 38 MB"];
  if (isIOSVersionLessThan(8))
  {
    UIActionSheet * actionSheet = [[UIActionSheet alloc] initWithTitle:message
                                                              delegate:self
                                                     cancelButtonTitle:nil
                                                destructiveButtonTitle:nil
                                                     otherButtonTitles:nil];
    [actionSheet addButtonWithTitle:downloadActionTitle];
    [actionSheet addButtonWithTitle:self.showActionTitle];
    if (!IPAD)
    {
      [actionSheet addButtonWithTitle:self.cancelActionTitle];
      actionSheet.cancelButtonIndex = actionSheet.numberOfButtons - 1;
    }
    UITableViewCell * cell = [tableView cellForRowAtIndexPath:indexPath];
    [actionSheet showFromRect:cell.frame inView:cell.superview animated:YES];
  }
  else
  {
    UIAlertController * alertController =
        [UIAlertController alertControllerWithTitle:title
                                            message:message
                                     preferredStyle:UIAlertControllerStyleActionSheet];
    UIAlertAction * downloadAction = [UIAlertAction actionWithTitle:downloadActionTitle
                                                              style:UIAlertActionStyleDefault
                                                            handler:self.downloadAction];
    UIAlertAction * showAction = [UIAlertAction actionWithTitle:self.showActionTitle
                                                          style:UIAlertActionStyleDefault
                                                        handler:self.showAction];
    UIAlertAction * cancelAction = [UIAlertAction actionWithTitle:self.cancelActionTitle
                                                            style:UIAlertActionStyleCancel
                                                          handler:nil];
    [alertController addAction:downloadAction];
    [alertController addAction:showAction];
    [alertController addAction:cancelAction];
    [self presentViewController:alertController animated:YES completion:nil];
  }
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
  };

  self.showAction = ^(UIAlertAction * action)
  {
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

@end
