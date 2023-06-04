#import "MWMCollectionViewController.h"
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

#pragma mark - Properties

- (BOOL)hasNavigationBar { return YES; }
- (MWMAlertViewController *)alertController
{
  if (!_alertController)
    _alertController = [[MWMAlertViewController alloc] initWithViewController:self];
  return _alertController;
}

@end
