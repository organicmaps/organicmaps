#import "Common.h"
#import "MWMAlert.h"
#import "MWMAlertViewController.h"
#import "MWMDefaultAlert.h"
#import "MWMDownloadTransitMapAlert.h"
#import "MWMFacebookAlert.h"
#import "MWMLocationAlert.h"
#import "MWMPedestrianShareAlert.h"
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

+ (MWMAlert *)noWiFiAlertWithName:(NSString *)name downloadBlock:(TMWMVoidBlock)block
{
  return [MWMDefaultAlert noWiFiAlertWithName:name downloadBlock:block];
}

+ (MWMAlert *)noConnectionAlert
{
  return [MWMDefaultAlert noConnectionAlert];
}

+ (MWMAlert *)locationServiceNotSupportedAlert
{
  return [MWMDefaultAlert locationServiceNotSupportedAlert];
}

+ (MWMAlert *)downloaderAlertWithAbsentCountries:(vector<storage::TIndex> const &)countries
                                          routes:(vector<storage::TIndex> const &)routes
                                            code:(routing::IRouter::ResultCode)code
{
  return [MWMDownloadTransitMapAlert downloaderAlertWithMaps:countries routes:routes code:code];
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
      return [MWMDefaultAlert internalErrorAlert];
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

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
// Should override this method if you want custom relayout after rotation.
}

- (void)close
{
  [self.alertController closeAlertWithCompletion:^
  {
    [self removeFromSuperview];
  }];
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
  if (isIOSVersionLessThan(8) && [self respondsToSelector:@selector(setTransform:)])
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
