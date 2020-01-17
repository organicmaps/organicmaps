#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMAlertViewController.h"
#import "MWMViewController.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

@interface MWMViewController ()

@property (nonatomic, readwrite) MWMAlertViewController * alertController;

@end

@implementation MWMViewController

- (NSString *)localizedTitle
{
  return L(self.title);
}

- (void)setLocalizedTitle:(NSString *)localizedTitle
{
  self.title = L(localizedTitle);
}

- (BOOL)prefersStatusBarHidden
{
  return NO;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self.navigationController.navigationBar setTranslucent:NO];
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
