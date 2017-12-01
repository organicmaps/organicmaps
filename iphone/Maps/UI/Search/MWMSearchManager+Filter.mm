#import "MWMSearch.h"
#import "MWMSearchFilterTransitioningManager.h"
#import "MWMSearchFilterViewController.h"
#import "MWMSearchManager+Filter.h"
#import "Statistics.h"

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

  [self configNavigationBar:navController.navigationBar];
  [self configNavigationItem:navController.topViewController.navigationItem];

  [ownerController presentViewController:navController animated:YES completion:nil];
}

- (IBAction)clearFilter { [MWMSearch clearFilter]; }
- (void)configNavigationBar:(UINavigationBar *)navBar
{
  if (IPAD)
  {
    UIColor * white = [UIColor white];
    navBar.tintColor = white;
    navBar.barTintColor = white;
    navBar.translucent = NO;
  }
  navBar.titleTextAttributes = @{
    NSForegroundColorAttributeName: IPAD ? [UIColor blackPrimaryText] : [UIColor whiteColor],
    NSFontAttributeName: [UIFont bold17]
  };
}

- (void)configNavigationItem:(UINavigationItem *)navItem
{
  UIFont * textFont = [UIFont regular17];

  UIColor * normalStateColor = IPAD ? [UIColor linkBlue] : [UIColor whiteColor];
  UIColor * highlightedStateColor = IPAD ? [UIColor linkBlueHighlighted] : [UIColor whiteColor];
  UIColor * disabledStateColor = [UIColor lightGrayColor];

  navItem.title = L(@"booking_filters");
  navItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithTitle:L(@"booking_filters_reset")
                                                                style:UIBarButtonItemStylePlain
                                                               target:self
                                                               action:@selector(resetAction)];
  [navItem.rightBarButtonItem setTitleTextAttributes:@{
    NSForegroundColorAttributeName: normalStateColor,
    NSFontAttributeName: textFont
  }
                                            forState:UIControlStateNormal];
  [navItem.rightBarButtonItem setTitleTextAttributes:@{
    NSForegroundColorAttributeName: highlightedStateColor,
  }
                                            forState:UIControlStateHighlighted];
  [navItem.rightBarButtonItem setTitleTextAttributes:@{
    NSForegroundColorAttributeName: disabledStateColor,
  }
                                            forState:UIControlStateDisabled];

  navItem.leftBarButtonItem =
      [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel
                                                    target:self
                                                    action:@selector(closeAction)];

  [navItem.leftBarButtonItem setTitleTextAttributes:@{
    NSForegroundColorAttributeName: normalStateColor,
    NSFontAttributeName: textFont
  }
                                           forState:UIControlStateNormal];
  [navItem.leftBarButtonItem setTitleTextAttributes:@{
    NSForegroundColorAttributeName: highlightedStateColor,
  }
                                           forState:UIControlStateHighlighted];

  [navItem.leftBarButtonItem setTitleTextAttributes:@{
    NSForegroundColorAttributeName: disabledStateColor,
  }
                                           forState:UIControlStateDisabled];
}

#pragma mark - Actions

- (void)closeAction
{
  [Statistics logEvent:kStatSearchFilterCancel withParameters:@{kStatCategory: kStatHotel}];
  [self.ownerController dismissViewControllerAnimated:YES completion:nil];
}

- (void)resetAction
{
  [Statistics logEvent:kStatSearchFilterReset withParameters:@{kStatCategory: kStatHotel}];
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
