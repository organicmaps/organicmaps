#import "Common.h"
#import "MWMAlertViewController.h"
#import "MWMDownloadTransitMapAlert.h"

static NSString * const kAlertControllerNibIdentifier = @"MWMAlertViewController";

@interface MWMAlertViewController () <UIGestureRecognizerDelegate>

@property (weak, nonatomic, readwrite) UIViewController * ownerViewController;

@end

@implementation MWMAlertViewController

- (nonnull instancetype)initWithViewController:(nonnull UIViewController *)viewController
{
  self = [super initWithNibName:kAlertControllerNibIdentifier bundle:nil];
  if (self)
    self.ownerViewController = viewController;
  return self;
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
  MWMAlert * alert = self.view.subviews.firstObject;
  [alert rotate:toInterfaceOrientation duration:duration];
}

#pragma mark - Actions

- (void)presentRateAlert
{
  [self displayAlert:MWMAlert.rateAlert];
}

- (void)presentLocationAlert
{
  NSString * title = L(@"location_is_disabled_long_text");
  NSString * cancel = L(@"cancel");
  NSString * openSettings = L(@"settings");
  if (isIOS7)
  {
    UIAlertView * alertView = [[UIAlertView alloc] initWithTitle:title message:nil delegate:nil cancelButtonTitle:cancel otherButtonTitles:nil];
    [alertView show];
    return;
  }
  UIAlertController * alertController = [UIAlertController alertControllerWithTitle:title message:nil preferredStyle:UIAlertControllerStyleAlert];
  UIAlertAction * cancelAction = [UIAlertAction actionWithTitle:cancel style:UIAlertActionStyleCancel handler:nil];
  UIAlertAction * openSettingsAction = [UIAlertAction actionWithTitle:openSettings style:UIAlertActionStyleDefault handler:^(UIAlertAction * action)
  {
    [self openSettings];
  }];
  [alertController addAction:cancelAction];
  [alertController addAction:openSettingsAction];
  [self.ownerViewController presentViewController:alertController animated:YES completion:nil];
//  dispatch_async(dispatch_get_main_queue(), ^
//  {
//    // @TODO Remove dispatch on LocationManager -> MWMLocationManager
//    // Test case when location is denied by user on app launch/relaunch
//    [self displayAlert:MWMAlert.locationAlert];
//  });
}

- (void)presentPoint2PointAlertWithOkBlock:(nonnull TMWMVoidBlock)block needToRebuild:(BOOL)needToRebuild
{
  [self displayAlert:[MWMAlert point2PointAlertWithOkBlock:block needToRebuild:needToRebuild]];
}

- (void)presentNeedMigrationAlertWithOkBlock:(nonnull TMWMVoidBlock)block
{
  [self displayAlert:[MWMAlert needMigrationAlertWithOkBlock:block]];
}

- (void)presentFacebookAlert
{
  [self displayAlert:MWMAlert.facebookAlert];
}

- (void)presentLocationServiceNotSupportedAlert
{
  [self displayAlert:MWMAlert.locationServiceNotSupportedAlert];
}

- (void)presentNoConnectionAlert
{
  [self displayAlert:MWMAlert.noConnectionAlert];
}

- (void)presentNoWiFiAlertWithName:(nonnull NSString *)name downloadBlock:(nullable TMWMVoidBlock)block
{
  [self displayAlert:[MWMAlert noWiFiAlertWithName:name downloadBlock:block]];
}

- (void)presentPedestrianToastAlert:(BOOL)isFirstLaunch
{
  [self displayAlert:[MWMAlert pedestrianToastShareAlert:isFirstLaunch]];
}

- (void)presentInternalErrorAlert
{
  [self displayAlert:[MWMAlert internalErrorAlert]];
}

- (void)presentInvalidUserNameOrPasswordAlert
{
  [self displayAlert:[MWMAlert invalidUserNameOrPasswordAlert]];
}

- (void)presentDownloaderAlertWithCountries:(vector<storage::TIndex> const &)countries
                                     routes:(vector<storage::TIndex> const &)routes
                                       code:(routing::IRouter::ResultCode)code
                                      block:(TMWMVoidBlock)block
{
  [self displayAlert:[MWMAlert downloaderAlertWithAbsentCountries:countries routes:routes code:code block:block]];
}

- (void)presentRoutingDisclaimerAlert
{
  [self displayAlert:[MWMAlert routingDisclaimerAlertWithInitialOrientation:self.ownerViewController.interfaceOrientation]];
}

- (void)presentDisabledLocationAlert
{
  [self displayAlert:MWMAlert.disabledLocationAlert];
}

- (void)presentAlert:(routing::IRouter::ResultCode)type
{
  [self displayAlert:[MWMAlert alert:type]];
}

- (void)displayAlert:(MWMAlert *)alert
{
  alert.alertController = self;
  [self.ownerViewController addChildViewController:self];
  self.view.alpha = 0.;
  alert.alpha = 0.;
  if (!isIOS7)
  {
    CGFloat const scale = 1.1;
    alert.transform = CGAffineTransformMakeScale(scale, scale);
  }
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.view.alpha = 1.;
    alert.alpha = 1.;
    if (!isIOS7)
      alert.transform = CGAffineTransformIdentity;
  }];
}

- (void)closeAlertWithCompletion:(nullable TMWMVoidBlock)completion
{
  MWMAlert * alert = self.view.subviews.firstObject;
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    alert.alpha = 0.;
    self.view.alpha = 0.;
  }
  completion:^(BOOL finished)
  {
    if (completion)
      completion();
    [self.view removeFromSuperview];
    [self removeFromParentViewController];
  }];
}

- (void)openSettings
{
  NSURL * url = [NSURL URLWithString:UIApplicationOpenSettingsURLString];
  UIApplication * a = [UIApplication sharedApplication];
  if ([a canOpenURL:url])
    [a openURL:url];
}

@end
