#import "Common.h"
#import "MWMAlert.h"
#import "MWMAlertViewController.h"
#import "MWMDefaultAlert.h"
#import "MWMDownloadTransitMapAlert.h"
#import "MWMEditorViralAlert.h"
#import "MWMFacebookAlert.h"
#import "MWMLocationAlert.h"
#import "MWMOsmAuthAlert.h"
#import "MWMPedestrianShareAlert.h"
#import "MWMPlaceDoesntExistAlert.h"
#import "MWMRateAlert.h"
#import "MWMRoutingDisclaimerAlert.h"

@implementation MWMAlert

+ (MWMAlert *)rateAlert
{
  return [MWMRateAlert alert];
}

+ (MWMAlert *)locationAlert
{
  return [MWMLocationAlert alert];
}

+ (MWMAlert *)facebookAlert
{
  return [MWMFacebookAlert alert];
}

+ (MWMAlert *)point2PointAlertWithOkBlock:(TMWMVoidBlock)block needToRebuild:(BOOL)needToRebuild
{
  return [MWMDefaultAlert point2PointAlertWithOkBlock:block needToRebuild:needToRebuild];
}

+ (MWMAlert *)routingDisclaimerAlertWithInitialOrientation:(UIInterfaceOrientation)orientation
{
  return [MWMRoutingDisclaimerAlert alertWithInitialOrientation:orientation];
}

+ (MWMAlert *)disabledLocationAlert
{
  return [MWMDefaultAlert disabledLocationAlert];
}

+ (MWMAlert *)noWiFiAlertWithOkBlock:(TMWMVoidBlock)okBlock
{
  return [MWMDefaultAlert noWiFiAlertWithOkBlock:okBlock];
}

+ (MWMAlert *)noConnectionAlert
{
  return [MWMDefaultAlert noConnectionAlert];
}

+ (MWMAlert *)migrationProhibitedAlert
{
  return [MWMDefaultAlert migrationProhibitedAlert];
}

+ (MWMAlert *)unsavedEditsAlertWithOkBlock:(TMWMVoidBlock)okBlock
{
  return [MWMDefaultAlert unsavedEditsAlertWithOkBlock:okBlock];
}

+ (MWMAlert *)locationServiceNotSupportedAlert
{
  return [MWMDefaultAlert locationServiceNotSupportedAlert];
}

+ (MWMAlert *)locationNotFoundAlertWithOkBlock:(TMWMVoidBlock)okBlock cancelBlock:(TMWMVoidBlock)cancelBlock
{
  return [MWMDefaultAlert locationNotFoundAlertWithOkBlock:okBlock cancelBlock:cancelBlock];
}

+ (MWMAlert *)routingMigrationAlertWithOkBlock:(TMWMVoidBlock)okBlock
{
  return [MWMDefaultAlert routingMigrationAlertWithOkBlock:okBlock];
}

+ (MWMAlert *)downloaderAlertWithAbsentCountries:(storage::TCountriesVec const &)countries
                                            code:(routing::IRouter::ResultCode)code
                                     cancelBlock:(TMWMVoidBlock)cancelBlock
                                   downloadBlock:(TMWMDownloadBlock)downloadBlock
                           downloadCompleteBlock:(TMWMVoidBlock)downloadCompleteBlock
{
  return [MWMDownloadTransitMapAlert downloaderAlertWithMaps:countries
                                                        code:code
                                                 cancelBlock:cancelBlock
                                               downloadBlock:downloadBlock
                                       downloadCompleteBlock:downloadCompleteBlock];
}

+ (MWMAlert *)alert:(routing::IRouter::ResultCode)type
{
  switch (type)
  {
    case routing::IRouter::NoCurrentPosition:
      return [MWMDefaultAlert noCurrentPositionAlert];
    case routing::IRouter::StartPointNotFound:
      return [MWMDefaultAlert startPointNotFoundAlert];
    case routing::IRouter::EndPointNotFound:
      return [MWMDefaultAlert endPointNotFoundAlert];
    case routing::IRouter::PointsInDifferentMWM:
      return [MWMDefaultAlert pointsInDifferentMWMAlert];
    case routing::IRouter::RouteNotFound:
    case routing::IRouter::InconsistentMWMandRoute:
      return [MWMDefaultAlert routeNotFoundAlert];
    case routing::IRouter::RouteFileNotExist:
    case routing::IRouter::FileTooOld:
      return [MWMDefaultAlert routeFileNotExistAlert];
    case routing::IRouter::InternalError:
      return [MWMDefaultAlert internalRoutingErrorAlert];
    case routing::IRouter::Cancelled:
    case routing::IRouter::NoError:
    case routing::IRouter::NeedMoreMaps:
      return nil;
  }
}

+ (MWMAlert *)pedestrianToastShareAlert:(BOOL)isFirstLaunch
{
  return [MWMPedestrianShareAlert alert:isFirstLaunch];
}

+ (MWMAlert *)incorrectFeauturePositionAlert
{
  return [MWMDefaultAlert incorrectFeauturePositionAlert];
}

+ (MWMAlert *)internalErrorAlert
{
  return [MWMDefaultAlert internalErrorAlert];
}

+ (MWMAlert *)notEnoughSpaceAlert
{
  return [MWMDefaultAlert notEnoughSpaceAlert];
}

+ (MWMAlert *)invalidUserNameOrPasswordAlert
{
  return [MWMDefaultAlert invalidUserNameOrPasswordAlert];
}

+ (MWMAlert *)disableAutoDownloadAlertWithOkBlock:(TMWMVoidBlock)okBlock
{
  return [MWMDefaultAlert disableAutoDownloadAlertWithOkBlock:okBlock];
}

+ (MWMAlert *)downloaderNoConnectionAlertWithOkBlock:(TMWMVoidBlock)okBlock cancelBlock:(TMWMVoidBlock)cancelBlock
{
  return [MWMDefaultAlert downloaderNoConnectionAlertWithOkBlock:okBlock cancelBlock:cancelBlock];
}

+ (MWMAlert *)downloaderNotEnoughSpaceAlert
{
  return [MWMDefaultAlert downloaderNotEnoughSpaceAlert];
}

+ (MWMAlert *)downloaderInternalErrorAlertWithOkBlock:(TMWMVoidBlock)okBlock cancelBlock:(TMWMVoidBlock)cancelBlock
{
  return [MWMDefaultAlert downloaderInternalErrorAlertWithOkBlock:okBlock cancelBlock:cancelBlock];
}

+ (MWMAlert *)downloaderNeedUpdateAlertWithOkBlock:(TMWMVoidBlock)okBlock
{
  return [MWMDefaultAlert downloaderNeedUpdateAlertWithOkBlock:okBlock];
}

+ (MWMAlert *)placeDoesntExistAlertWithBlock:(MWMStringBlock)block
{
  return [MWMPlaceDoesntExistAlert alertWithBlock:block];
}

+ (MWMAlert *)resetChangesAlertWithBlock:(TMWMVoidBlock)block
{
  return [MWMDefaultAlert resetChangesAlertWithBlock:block];
}

+ (MWMAlert *)deleteFeatureAlertWithBlock:(TMWMVoidBlock)block
{
  return [MWMDefaultAlert deleteFeatureAlertWithBlock:block];
}

+ (MWMAlert *)editorViralAlert
{
  return [MWMEditorViralAlert alert];
}

+ (MWMAlert *)osmAuthAlert
{
  return [MWMOsmAuthAlert alert];
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
// Should override this method if you want custom relayout after rotation.
}

- (void)close
{
  [self.alertController closeAlert];
}

- (void)setNeedsCloseAlertAfterEnterBackground
{
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationDidEnterBackground) name:UIApplicationDidEnterBackgroundNotification object:nil];
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)applicationDidEnterBackground
{
// Should close alert when application entered background.
  [self close];
}

- (void)rotate:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
  if (isIOS7 && [self respondsToSelector:@selector(setTransform:)])
  {
    [UIView animateWithDuration:duration animations:^
    {
      self.transform = rotation(toInterfaceOrientation);
    }];
  }
  if ([self respondsToSelector:@selector(willRotateToInterfaceOrientation:)])
    [self willRotateToInterfaceOrientation:toInterfaceOrientation];
}

CGAffineTransform rotation(UIInterfaceOrientation orientation)
{
  switch (orientation)
  {
    case UIInterfaceOrientationLandscapeLeft:
      return CGAffineTransformMakeRotation(-M_PI_2);
    case UIInterfaceOrientationLandscapeRight:
      return CGAffineTransformMakeRotation(M_PI_2);
    case UIInterfaceOrientationPortraitUpsideDown:
      return CGAffineTransformMakeRotation(M_PI);
    case UIInterfaceOrientationUnknown:
    case UIInterfaceOrientationPortrait:
      return CGAffineTransformIdentity;
  }
}

- (void)addControllerViewToWindow
{
  UIWindow * window = UIApplication.sharedApplication.delegate.window;
  UIView * view = self.alertController.view;
  [window addSubview:view];
  view.frame = window.bounds;
}

- (void)setAlertController:(MWMAlertViewController *)alertController
{
  _alertController = alertController;
  UIView * view = alertController.view;
  UIView * ownerView = alertController.ownerViewController.view;
  view.frame = ownerView.bounds;
  [alertController.ownerViewController.view addSubview:view];
  [self addControllerViewToWindow];
  [self rotate:alertController.ownerViewController.interfaceOrientation duration:0.0];
  [view addSubview:self];
  self.frame = view.bounds;
}

@end
