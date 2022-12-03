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

// Fix table section header font color for all tables, including Setting and Route Options.
- (void)tableView:(UITableView *)tableView willDisplayHeaderView:(UIView *)view forSection:(NSInteger)section {
  if ([view isKindOfClass: [UITableViewHeaderFooterView class]]) {
    UITableViewHeaderFooterView * header = (UITableViewHeaderFooterView *)view;
    UIColor * sectionHeaderColor = [UIColor blackSecondaryText];
    @try {
      // iOS 14+
      ((UILabel *)[header.contentView valueForKey:@"textLabel"]).textColor = sectionHeaderColor;
    } @catch (id ex) {
      @try {
        // iOS 12 and 13.
        ((UILabel *)[header valueForKey:@"label"]).textColor = sectionHeaderColor;
      } @catch (id ex) {
        // A custom header view is used for the table.
      }
    }
  }
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
