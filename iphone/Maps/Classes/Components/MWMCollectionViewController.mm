#import "MWMCollectionViewController.h"
#import "3party/Alohalytics/src/alohalytics_objc.h"
#import "MWMAlertViewController.h"
#import "MapViewController.h"

@interface MWMCollectionViewController ()

@property(nonatomic, readwrite) MWMAlertViewController * alertController;

@end

@implementation MWMCollectionViewController

- (BOOL)prefersStatusBarHidden { return NO; }
- (void)mwm_refreshUI
{
  [self.navigationController.navigationBar mwm_refreshUI];
  MapViewController * mapViewController = [MapViewController controller];
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
  self.collectionView.backgroundColor = [UIColor pressBackground];
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

- (BOOL)hasNavigationBar { return YES; }
- (MWMAlertViewController *)alertController
{
  if (!_alertController)
    _alertController = [[MWMAlertViewController alloc] initWithViewController:self];
  return _alertController;
}

@end
