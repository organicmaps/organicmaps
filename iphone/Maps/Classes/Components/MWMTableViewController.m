#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMAlertViewController.h"
#import "MWMTableViewCell.h"
#import "MWMTableViewController.h"
#import "SwiftBridge.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

@interface MWMTableViewController ()

@property (nonatomic, readwrite) MWMAlertViewController * alertController;

@end

@implementation MWMTableViewController

- (BOOL)prefersStatusBarHidden
{
  return NO;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.tableView.insetsContentViewsToSafeArea = YES;
  self.tableView.styleName = @"TableView:PressBackground";
  [self.navigationController.navigationBar setTranslucent:NO];
  [self.tableView registerClass:[MWMTableViewCell class]
         forCellReuseIdentifier:[UITableViewCell className]];
  [self.tableView registerClass:[MWMTableViewSubtitleCell class]
         forCellReuseIdentifier:[MWMTableViewSubtitleCell className]];
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

@implementation UITableView (MWMTableViewController)

- (UITableViewCell *)dequeueDefaultCellForIndexPath:(NSIndexPath *)indexPath {
  return [self dequeueReusableCellWithIdentifier:UITableViewCell.className forIndexPath:indexPath];
}

@end
