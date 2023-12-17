#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMAlertViewController.h"
#import "MWMTableViewCell.h"
#import "MWMTableViewController.h"
#import "SwiftBridge.h"


@interface MWMTableViewController ()

@property (nonatomic, readwrite) MWMAlertViewController * alertController;

@end

@implementation MWMTableViewController

- (BOOL)prefersStatusBarHidden
{
  return NO;
}

- (void)tableView:(UITableView *)tableView willDisplayHeaderView:(UIView *)view forSection:(NSInteger)section
{
  [self fixHeaderAndFooterFontsInDarkMode:view];
}

- (void)tableView:(UITableView *)tableView willDisplayFooterView:(UIView *)view forSection:(NSInteger)section
{
  [self fixHeaderAndFooterFontsInDarkMode:view];
}

// Fix table section header font color for all tables, including Setting and Route Options.
- (void)fixHeaderAndFooterFontsInDarkMode:(UIView*)headerView {
  if ([headerView isKindOfClass: [UITableViewHeaderFooterView class]]) {
    UITableViewHeaderFooterView* header = (UITableViewHeaderFooterView *)headerView;
    header.textLabel.textColor = [UIColor blackSecondaryText];
    if (self.tableView.style == UITableViewStyleGrouped) {
      header.detailTextLabel.textColor = [UIColor blackSecondaryText];
    }
  }
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
