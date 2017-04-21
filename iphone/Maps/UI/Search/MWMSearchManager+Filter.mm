#import "MWMSearch.h"
#import "MWMSearchFilterTransitioningManager.h"
#import "MWMSearchFilterViewController.h"
#import "MWMSearchManager+Filter.h"

@interface MWMSearchManager ()<UIPopoverPresentationControllerDelegate>

@property(weak, nonatomic, readonly) UIViewController * ownerController;
@property(weak, nonatomic) IBOutlet UIButton * actionBarViewFilterButton;

@property(nonatomic) MWMSearchFilterTransitioningManager * filterTransitioningManager;

@end

@implementation MWMSearchManager (Filter)

- (IBAction)updateFilter
{
  MWMSearchFilterViewController * filter = [MWMSearch getFilter];
  UINavigationController * navController =
      [[UINavigationController alloc] initWithRootViewController:filter];
  UIViewController * ownerController = self.ownerController;

  if (IPAD)
  {
    navController.modalPresentationStyle = UIModalPresentationPopover;
    UIPopoverPresentationController * popover = navController.popoverPresentationController;
    popover.sourceView = self.actionBarViewFilterButton;
    popover.sourceRect = self.actionBarViewFilterButton.bounds;
    popover.permittedArrowDirections = UIPopoverArrowDirectionLeft;
    popover.delegate = self;
  }
  else
  {
    navController.modalPresentationStyle = UIModalPresentationCustom;
    self.filterTransitioningManager = [[MWMSearchFilterTransitioningManager alloc] init];
    ownerController.transitioningDelegate = self.filterTransitioningManager;
    navController.transitioningDelegate = self.filterTransitioningManager;
  }

  [self configNavigationBar:navController.navigationBar];
  [self configNavigationItem:navController.topViewController.navigationItem];

  [ownerController presentViewController:navController animated:YES completion:nil];
}

- (IBAction)clearFilter { [MWMSearch clearFilter]; }
- (void)configNavigationBar:(UINavigationBar *)navBar
{
  UIColor * white = [UIColor white];
  navBar.tintColor = white;
  navBar.barTintColor = white;
  [navBar setBackgroundImage:nil forBarMetrics:UIBarMetricsDefault];
  navBar.shadowImage = [UIImage imageWithColor:[UIColor fadeBackground]];
  navBar.titleTextAttributes = @{
    NSForegroundColorAttributeName : [UIColor blackPrimaryText],
    NSFontAttributeName : [UIFont regular17]
  };
  navBar.translucent = NO;
}

- (void)configNavigationItem:(UINavigationItem *)navItem
{
  UIFont * regular17 = [UIFont regular17];

  UIColor * linkBlue = [UIColor linkBlue];
  UIColor * linkBlueHighlighted = [UIColor linkBlueHighlighted];
  UIColor * lightGrayColor = [UIColor lightGrayColor];

  navItem.title = L(@"booking_filters");
  navItem.rightBarButtonItem =
      [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone
                                                    target:self
                                                    action:@selector(doneAction)];
  [navItem.rightBarButtonItem setTitleTextAttributes:@{
    NSForegroundColorAttributeName : linkBlue,
    NSFontAttributeName : regular17
  }
                                            forState:UIControlStateNormal];
  [navItem.rightBarButtonItem setTitleTextAttributes:@{
    NSForegroundColorAttributeName : linkBlueHighlighted,
  }
                                            forState:UIControlStateHighlighted];
  [navItem.rightBarButtonItem setTitleTextAttributes:@{
    NSForegroundColorAttributeName : lightGrayColor,
  }
                                            forState:UIControlStateDisabled];

  navItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithTitle:L(@"booking_filters_reset")
                                                               style:UIBarButtonItemStylePlain
                                                              target:self
                                                              action:@selector(resetAction)];
  [navItem.leftBarButtonItem setTitleTextAttributes:@{
    NSForegroundColorAttributeName : linkBlue,
    NSFontAttributeName : regular17
  }
                                           forState:UIControlStateNormal];
  [navItem.leftBarButtonItem setTitleTextAttributes:@{
    NSForegroundColorAttributeName : linkBlueHighlighted,
  }
                                           forState:UIControlStateHighlighted];

  [navItem.leftBarButtonItem setTitleTextAttributes:@{
    NSForegroundColorAttributeName : lightGrayColor,
  }
                                           forState:UIControlStateDisabled];
}

#pragma mark - Actions

- (void)doneAction
{
  [MWMSearch update];
  [self.ownerController dismissViewControllerAnimated:YES completion:nil];
}

- (void)resetAction
{
  [self clearFilter];
}

#pragma mark - UIPopoverPresentationControllerDelegate

- (BOOL)popoverPresentationControllerShouldDismissPopover:
    (UIPopoverPresentationController *)popoverPresentationController
{
  [MWMSearch update];
  return YES;
}

@end
