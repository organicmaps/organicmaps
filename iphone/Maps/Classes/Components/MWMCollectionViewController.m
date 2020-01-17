#import "MWMCollectionViewController.h"
#import "3party/Alohalytics/src/alohalytics_objc.h"
#import "MWMAlertViewController.h"
#import "MapViewController.h"
#import "SwiftBridge.h"

@interface MWMCollectionViewController ()

@property(nonatomic, readwrite) MWMAlertViewController * alertController;

@end

@implementation MWMCollectionViewController

- (BOOL)prefersStatusBarHidden { return NO; }

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.collectionView.styleName = @"PressBackground";
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
