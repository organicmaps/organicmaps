#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMAlertViewController.h"
#import "MWMTableViewCell.h"
#import "MWMTableViewController.h"
#import "UIColor+MapsMeColor.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

@interface MWMTableViewController ()

@property (nonatomic, readwrite) MWMAlertViewController * alertController;

@end

@implementation MWMTableViewController

- (BOOL)prefersStatusBarHidden
{
  return NO;
}

- (void)mwm_refreshUI
{
  [self.navigationController.navigationBar mwm_refreshUI];
  MapViewController * mapViewController = [MapsAppDelegate theApp].mapViewController;
  for (UIViewController * vc in self.navigationController.viewControllers.reverseObjectEnumerator)
  {
    if (![vc isEqual:mapViewController])
      [vc.view mwm_refreshUI];
  }
  [mapViewController mwm_refreshUI];
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.tableView.backgroundColor = [UIColor pressBackground];
  self.tableView.separatorColor = [UIColor blackDividers];
  [self.navigationController.navigationBar setTranslucent:NO];
  [self.tableView registerClass:[MWMTableViewCell class] forCellReuseIdentifier:[UITableViewCell className]];
}

- (void)viewWillAppear:(BOOL)animated
{
  [Alohalytics logEvent:@"$viewWillAppear" withValue:NSStringFromClass([self class])];
  [super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [Alohalytics logEvent:@"$viewWillDisappear" withValue:NSStringFromClass([self class])];
  [super viewWillDisappear:animated];
}

#pragma mark - Properties

- (BOOL)hasNavigationBar
{
  return YES;
}

- (MWMAlertViewController *)alertController
{
  if (!_alertController)
    _alertController = [[MWMAlertViewController alloc] initWithViewController:self];
  return _alertController;
}

@end
