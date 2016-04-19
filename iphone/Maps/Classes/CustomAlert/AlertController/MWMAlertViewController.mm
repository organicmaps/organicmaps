#import "Common.h"
#import "MWMAlertViewController.h"
#import "MWMDownloadTransitMapAlert.h"
#import "MapsAppDelegate.h"

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
  [self displayAlert:[MWMAlert locationAlert]];
}

- (void)presentPoint2PointAlertWithOkBlock:(nonnull TMWMVoidBlock)okBlock needToRebuild:(BOOL)needToRebuild
{
  [self displayAlert:[MWMAlert point2PointAlertWithOkBlock:okBlock needToRebuild:needToRebuild]];
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
  [self displayAlert:[MWMAlert noConnectionAlert]];
}

- (void)presentMigrationProhibitedAlert
{
  [self displayAlert:[MWMAlert migrationProhibitedAlert]];
}

- (void)presentUnsavedEditsAlertWithOkBlock:(nonnull TMWMVoidBlock)okBlock
{
  [self displayAlert:[MWMAlert unsavedEditsAlertWithOkBlock:okBlock]];
}

- (void)presentNoWiFiAlertWithOkBlock:(nullable TMWMVoidBlock)okBlock
{
  [self displayAlert:[MWMAlert noWiFiAlertWithOkBlock:okBlock]];
}

- (void)presentPedestrianToastAlert:(BOOL)isFirstLaunch
{
  [self displayAlert:[MWMAlert pedestrianToastShareAlert:isFirstLaunch]];
}

- (void)presentIncorrectFeauturePositionAlert
{
  [self displayAlert:[MWMAlert incorrectFeauturePositionAlert]];
}

- (void)presentInternalErrorAlert
{
  [self displayAlert:[MWMAlert internalErrorAlert]];
}

- (void)presentNotEnoughSpaceAlert
{
  [self displayAlert:[MWMAlert notEnoughSpaceAlert]];
}

- (void)presentInvalidUserNameOrPasswordAlert
{
  [self displayAlert:[MWMAlert invalidUserNameOrPasswordAlert]];
}

- (void)presentRoutingMigrationAlertWithOkBlock:(TMWMVoidBlock)okBlock
{
  [self displayAlert:[MWMAlert routingMigrationAlertWithOkBlock:okBlock]];
}

- (void)presentDownloaderAlertWithCountries:(storage::TCountriesVec const &)countries
                                       code:(routing::IRouter::ResultCode)code
                                cancelBlock:(TMWMVoidBlock)cancelBlock
                              downloadBlock:(TMWMDownloadBlock)downloadBlock
                      downloadCompleteBlock:(TMWMVoidBlock)downloadCompleteBlock
{
  [self displayAlert:[MWMAlert downloaderAlertWithAbsentCountries:countries
                                                             code:code
                                                      cancelBlock:cancelBlock
                                                    downloadBlock:downloadBlock
                                            downloadCompleteBlock:downloadCompleteBlock]];
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
  [MapsAppDelegate.theApp.window endEditing:YES];
}

- (void)presentDisableAutoDownloadAlertWithOkBlock:(nonnull TMWMVoidBlock)okBlock
{
  [self displayAlert:[MWMAlert disableAutoDownloadAlertWithOkBlock:okBlock]];
}

- (void)presentDownloaderNoConnectionAlertWithOkBlock:(nonnull TMWMVoidBlock)okBlock cancelBlock:(nonnull TMWMVoidBlock)cancelBlock
{
  [self displayAlert:[MWMAlert downloaderNoConnectionAlertWithOkBlock:okBlock cancelBlock:cancelBlock]];
}

- (void)presentDownloaderNotEnoughSpaceAlert
{
  [self displayAlert:[MWMAlert downloaderNotEnoughSpaceAlert]];
}

- (void)presentDownloaderInternalErrorAlertWithOkBlock:(nonnull TMWMVoidBlock)okBlock cancelBlock:(nonnull TMWMVoidBlock)cancelBlock
{
  [self displayAlert:[MWMAlert downloaderInternalErrorAlertWithOkBlock:okBlock cancelBlock:cancelBlock]];
}

- (void)presentDownloaderNeedUpdateAlertWithOkBlock:(nonnull TMWMVoidBlock)okBlock
{
  [self displayAlert:[MWMAlert downloaderNeedUpdateAlertWithOkBlock:okBlock]];
}

- (void)presentPlaceDoesntExistAlertWithBlock:(MWMStringBlock)block
{
  [self displayAlert:[MWMAlert placeDoesntExistAlertWithBlock:block]];
}

- (void)presentResetChangesAlertWithBlock:(TMWMVoidBlock)block
{
  [self displayAlert:[MWMAlert resetChangesAlertWithBlock:block]];
}

- (void)presentDeleteFeatureAlertWithBlock:(TMWMVoidBlock)block
{
  [self displayAlert:[MWMAlert deleteFeatureAlertWithBlock:block]];
}

- (void)presentEditorViralAlert
{
  [self displayAlert:[MWMAlert editorViralAlert]];
}

- (void)presentOsmAuthAlert
{
  [self displayAlert:[MWMAlert osmAuthAlert]];
}

- (void)closeAlert
{
  NSArray * subviews = self.view.subviews;
  MWMAlert * alert = subviews.firstObject;
  BOOL const isLastAlert = (subviews.count == 1);
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    alert.alpha = 0.;
    if (isLastAlert)
      self.view.alpha = 0.;
  }
  completion:^(BOOL finished)
  {
    [alert removeFromSuperview];
    if (isLastAlert)
    {
      [self.view removeFromSuperview];
      [self removeFromParentViewController];
    }
  }];
}

@end
