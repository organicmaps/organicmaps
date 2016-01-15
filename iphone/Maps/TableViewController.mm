#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMTableViewCell.h"
#import "TableViewController.h"
#import "UIColor+MapsMeColor.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

@implementation TableViewController

- (BOOL)prefersStatusBarHidden
{
  return NO;
}

- (void)refresh
{
  [self.navigationController.navigationBar refresh];
  for (UIViewController * vc in self.navigationController.viewControllers.reverseObjectEnumerator)
  {
    if (![vc isKindOfClass:[MapViewController class]])
      [vc.view refresh];
  }
  if (![self isKindOfClass:[MapViewController class]])
    [[MapsAppDelegate theApp].mapViewController refresh];
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

@end
