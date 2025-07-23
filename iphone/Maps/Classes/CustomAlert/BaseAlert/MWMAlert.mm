#import "MWMAlert+CPP.h"
#import "MWMAlertViewController.h"
#import "MWMDefaultAlert.h"
#import "MWMDownloadTransitMapAlert.h"
#import "MWMEditorViralAlert.h"
#import "MWMLocationAlert.h"
#import "MWMOsmAuthAlert.h"
#import "MWMOsmReauthAlert.h"
#import "MWMPlaceDoesntExistAlert.h"
#import "MWMRoutingDisclaimerAlert.h"

#import "SwiftBridge.h"

@implementation MWMAlert

+ (MWMAlert *)locationAlertWithCancelBlock:(MWMVoidBlock)cancelBlock
{
  return [MWMLocationAlert alertWithCancelBlock:cancelBlock];
}

+ (MWMAlert *)point2PointAlertWithOkBlock:(MWMVoidBlock)block needToRebuild:(BOOL)needToRebuild
{
  return [MWMDefaultAlert point2PointAlertWithOkBlock:block needToRebuild:needToRebuild];
}

+ (MWMAlert *)routingDisclaimerAlertWithOkBlock:(MWMVoidBlock)block
{
  return [MWMRoutingDisclaimerAlert alertWithOkBlock:block];
}

+ (MWMAlert *)disabledLocationAlert
{
  return [MWMDefaultAlert disabledLocationAlert];
}

+ (MWMAlert *)locationServicesDisabledAlert
{
  return [LocationServicesDisabledAlert alert];
}

+ (MWMAlert *)noWiFiAlertWithOkBlock:(MWMVoidBlock)okBlock andCancelBlock:(MWMVoidBlock)cancelBlock
{
  return [MWMDefaultAlert noWiFiAlertWithOkBlock:okBlock andCancelBlock:cancelBlock];
}

+ (MWMAlert *)noConnectionAlert
{
  return [MWMDefaultAlert noConnectionAlert];
}

+ (MWMAlert *)deleteMapProhibitedAlert
{
  return [MWMDefaultAlert deleteMapProhibitedAlert];
}
+ (MWMAlert *)unsavedEditsAlertWithOkBlock:(MWMVoidBlock)okBlock
{
  return [MWMDefaultAlert unsavedEditsAlertWithOkBlock:okBlock];
}

+ (MWMAlert *)locationServiceNotSupportedAlert
{
  return [MWMDefaultAlert locationServiceNotSupportedAlert];
}

+ (MWMAlert *)downloaderAlertWithAbsentCountries:(storage::CountriesSet const &)countries
                                            code:(routing::RouterResultCode)code
                                     cancelBlock:(MWMVoidBlock)cancelBlock
                                   downloadBlock:(MWMDownloadBlock)downloadBlock
                           downloadCompleteBlock:(MWMVoidBlock)downloadCompleteBlock
{
  return [MWMDownloadTransitMapAlert downloaderAlertWithMaps:countries
                                                        code:code
                                                 cancelBlock:cancelBlock
                                               downloadBlock:downloadBlock
                                       downloadCompleteBlock:downloadCompleteBlock];
}

+ (MWMAlert *)alert:(routing::RouterResultCode)type
{
  switch (type)
  {
  case routing::RouterResultCode::NoCurrentPosition: return [MWMDefaultAlert noCurrentPositionAlert];
  case routing::RouterResultCode::StartPointNotFound: return [MWMDefaultAlert startPointNotFoundAlert];
  case routing::RouterResultCode::EndPointNotFound: return [MWMDefaultAlert endPointNotFoundAlert];
  case routing::RouterResultCode::PointsInDifferentMWM: return [MWMDefaultAlert pointsInDifferentMWMAlert];
  case routing::RouterResultCode::TransitRouteNotFoundNoNetwork:
    return [MWMDefaultAlert routeNotFoundNoPublicTransportAlert];
  case routing::RouterResultCode::TransitRouteNotFoundTooLongPedestrian:
    return [MWMDefaultAlert routeNotFoundTooLongPedestrianAlert];
  case routing::RouterResultCode::RouteNotFoundRedressRouteError:
  case routing::RouterResultCode::RouteNotFound:
  case routing::RouterResultCode::InconsistentMWMandRoute: return [MWMDefaultAlert routeNotFoundAlert];
  case routing::RouterResultCode::RouteFileNotExist:
  case routing::RouterResultCode::FileTooOld: return [MWMDefaultAlert routeFileNotExistAlert];
  case routing::RouterResultCode::InternalError: return [MWMDefaultAlert internalRoutingErrorAlert];
  case routing::RouterResultCode::Cancelled:
  case routing::RouterResultCode::NoError:
  case routing::RouterResultCode::HasWarnings:
  case routing::RouterResultCode::NeedMoreMaps: return nil;
  case routing::RouterResultCode::IntermediatePointNotFound: return [MWMDefaultAlert intermediatePointNotFoundAlert];
  }
}

+ (MWMAlert *)incorrectFeaturePositionAlert
{
  return [MWMDefaultAlert incorrectFeaturePositionAlert];
}

+ (MWMAlert *)notEnoughSpaceAlert
{
  return [MWMDefaultAlert notEnoughSpaceAlert];
}
+ (MWMAlert *)invalidUserNameOrPasswordAlert
{
  return [MWMDefaultAlert invalidUserNameOrPasswordAlert];
}

+ (MWMAlert *)downloaderNoConnectionAlertWithOkBlock:(MWMVoidBlock)okBlock cancelBlock:(MWMVoidBlock)cancelBlock
{
  return [MWMDefaultAlert downloaderNoConnectionAlertWithOkBlock:okBlock cancelBlock:cancelBlock];
}

+ (MWMAlert *)downloaderNotEnoughSpaceAlert
{
  return [MWMDefaultAlert downloaderNotEnoughSpaceAlert];
}

+ (MWMAlert *)downloaderInternalErrorAlertWithOkBlock:(MWMVoidBlock)okBlock cancelBlock:(MWMVoidBlock)cancelBlock
{
  return [MWMDefaultAlert downloaderInternalErrorAlertWithOkBlock:okBlock cancelBlock:cancelBlock];
}

+ (MWMAlert *)placeDoesntExistAlertWithBlock:(MWMStringBlock)block
{
  return [MWMPlaceDoesntExistAlert alertWithBlock:block];
}

+ (MWMAlert *)resetChangesAlertWithBlock:(MWMVoidBlock)block
{
  return [MWMDefaultAlert resetChangesAlertWithBlock:block];
}

+ (MWMAlert *)deleteFeatureAlertWithBlock:(MWMVoidBlock)block
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
+ (MWMAlert *)osmReauthAlert
{
  return [MWMOsmReauthAlert alert];
}
+ (MWMAlert *)personalInfoWarningAlertWithBlock:(MWMVoidBlock)block
{
  return [MWMDefaultAlert personalInfoWarningAlertWithBlock:block];
}

+ (MWMAlert *)trackWarningAlertWithCancelBlock:(MWMVoidBlock)block
{
  return [MWMDefaultAlert trackWarningAlertWithCancelBlock:block];
}

+ (MWMAlert *)infoAlert:(NSString *)title text:(NSString *)text
{
  return [MWMDefaultAlert infoAlert:title text:text];
}

+ (MWMAlert *)createBookmarkCategoryAlertWithMaxCharacterNum:(NSUInteger)max
                                             minCharacterNum:(NSUInteger)min
                                                    callback:(MWMCheckStringBlock)callback
{
  return [MWMBCCreateCategoryAlert alertWithMaxCharachersNum:max minCharactersNum:min callback:callback];
}

+ (MWMAlert *)spinnerAlertWithTitle:(NSString *)title cancel:(MWMVoidBlock)cancel
{
  return [MWMSpinnerAlert alertWithTitle:title cancel:cancel];
}

+ (MWMAlert *)bookmarkConversionErrorAlert
{
  return [MWMDefaultAlert bookmarkConversionErrorAlert];
}

+ (MWMAlert *)tagsLoadingErrorAlertWithOkBlock:okBlock cancelBlock:cancelBlock
{
  return [MWMDefaultAlert tagsLoadingErrorAlertWithOkBlock:okBlock cancelBlock:cancelBlock];
}

+ (MWMAlert *)bugReportAlertWithTitle:(NSString *)title
{
  return [MWMDefaultAlert bugReportAlertWithTitle:title];
}

+ (MWMAlert *)defaultAlertWithTitle:(NSString *)title
                            message:(NSString *)message
                   rightButtonTitle:(NSString *)rightButtonTitle
                    leftButtonTitle:(NSString *)leftButtonTitle
                  rightButtonAction:(MWMVoidBlock)action
{
  return [MWMDefaultAlert defaultAlertWithTitle:title
                                        message:message
                               rightButtonTitle:rightButtonTitle
                                leftButtonTitle:leftButtonTitle
                              rightButtonAction:action
                                            log:nil];
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
  // Should override this method if you want custom relayout after rotation.
}

- (void)close:(MWMVoidBlock)completion
{
  [self.alertController closeAlert:completion];
}
- (void)setNeedsCloseAlertAfterEnterBackground
{
  [NSNotificationCenter.defaultCenter addObserver:self
                                         selector:@selector(applicationDidEnterBackground)
                                             name:UIApplicationDidEnterBackgroundNotification
                                           object:nil];
}

- (void)dealloc
{
  [NSNotificationCenter.defaultCenter removeObserver:self];
}
- (void)applicationDidEnterBackground
{
  // Should close alert when application entered background.
  [self close:nil];
}

- (void)rotate:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
  if ([self respondsToSelector:@selector(willRotateToInterfaceOrientation:)])
    [self willRotateToInterfaceOrientation:toInterfaceOrientation];
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
  UIViewController * ownerViewController = alertController.ownerViewController;
  view.frame = ownerViewController.view.bounds;
  [ownerViewController.view addSubview:view];
  [self addControllerViewToWindow];
  auto const orientation = UIApplication.sharedApplication.statusBarOrientation;
  [self rotate:orientation duration:0.0];
  [view addSubview:self];
  self.frame = view.bounds;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.frame = self.superview.bounds;
  [super layoutSubviews];
}

- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection
{
  [super traitCollectionDidChange:previousTraitCollection];
  if (self.traitCollection.userInterfaceStyle != previousTraitCollection.userInterfaceStyle)
    [self updateViewStyle:self];
}

- (void)updateViewStyle:(UIView *)view
{
  if (!view)
    return;
  for (UIView * subview in view.subviews)
    [self updateViewStyle:subview];
  [view applyTheme];
}

@end
