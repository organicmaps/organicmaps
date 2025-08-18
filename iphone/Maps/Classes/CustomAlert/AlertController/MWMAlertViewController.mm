#import "MWMAlertViewController+CPP.h"
#import "MWMController.h"
#import "MWMDownloadTransitMapAlert.h"
#import "MWMLocationAlert.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "SwiftBridge.h"

static NSString * const kAlertControllerNibIdentifier = @"MWMAlertViewController";

@interface MWMAlertViewController () <UIGestureRecognizerDelegate>

@property(weak, nonatomic, readwrite) UIViewController * ownerViewController;

@end

@implementation MWMAlertViewController

+ (nonnull MWMAlertViewController *)activeAlertController
{
  UIViewController * tvc = [MapViewController sharedController];
  ASSERT([tvc conformsToProtocol:@protocol(MWMController)], ());
  UIViewController<MWMController> * mwmController = static_cast<UIViewController<MWMController> *>(tvc);
  return mwmController.alertController;
}

- (nonnull instancetype)initWithViewController:(nonnull UIViewController *)viewController
{
  self = [super initWithNibName:kAlertControllerNibIdentifier bundle:nil];
  if (self)
    _ownerViewController = viewController;
  return self;
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  auto const orient = size.width > size.height ? UIInterfaceOrientationLandscapeLeft : UIInterfaceOrientationPortrait;
  [coordinator
      animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context) {
        for (MWMAlert * alert in self.view.subviews)
          [alert rotate:orient duration:context.transitionDuration];
      }
                      completion:^(id<UIViewControllerTransitionCoordinatorContext> context){}];
}

#pragma mark - Actions

- (void)presentLocationAlertWithCancelBlock:(MWMVoidBlock)cancelBlock
{
  [self displayAlert:[MWMAlert locationAlertWithCancelBlock:cancelBlock]];
}
- (void)presentPoint2PointAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock needToRebuild:(BOOL)needToRebuild
{
  [self displayAlert:[MWMAlert point2PointAlertWithOkBlock:okBlock needToRebuild:needToRebuild]];
}

- (void)presentLocationServiceNotSupportedAlert
{
  [self displayAlert:[MWMAlert locationServiceNotSupportedAlert]];
}
- (void)presentNoConnectionAlert
{
  [self displayAlert:[MWMAlert noConnectionAlert]];
}

- (void)presentDeleteMapProhibitedAlert
{
  [self displayAlert:[MWMAlert deleteMapProhibitedAlert]];
}
- (void)presentUnsavedEditsAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock
{
  [self displayAlert:[MWMAlert unsavedEditsAlertWithOkBlock:okBlock]];
}

- (void)presentNoWiFiAlertWithOkBlock:(nullable MWMVoidBlock)okBlock andCancelBlock:(MWMVoidBlock)cancelBlock
{
  [self displayAlert:[MWMAlert noWiFiAlertWithOkBlock:okBlock andCancelBlock:cancelBlock]];
}

- (void)presentIncorrectFeauturePositionAlert
{
  [self displayAlert:[MWMAlert incorrectFeaturePositionAlert]];
}

- (void)presentNotEnoughSpaceAlert
{
  [self displayAlert:[MWMAlert notEnoughSpaceAlert]];
}
- (void)presentInvalidUserNameOrPasswordAlert
{
  [self displayAlert:[MWMAlert invalidUserNameOrPasswordAlert]];
}

- (void)presentDownloaderAlertWithCountries:(storage::CountriesSet const &)countries
                                       code:(routing::RouterResultCode)code
                                cancelBlock:(MWMVoidBlock)cancelBlock
                              downloadBlock:(MWMDownloadBlock)downloadBlock
                      downloadCompleteBlock:(MWMVoidBlock)downloadCompleteBlock
{
  [self displayAlert:[MWMAlert downloaderAlertWithAbsentCountries:countries
                                                             code:code
                                                      cancelBlock:cancelBlock
                                                    downloadBlock:downloadBlock
                                            downloadCompleteBlock:downloadCompleteBlock]];
}

- (void)presentRoutingDisclaimerAlertWithOkBlock:(MWMVoidBlock)block
{
  [self displayAlert:[MWMAlert routingDisclaimerAlertWithOkBlock:block]];
}

- (void)presentDisabledLocationAlert
{
  [self displayAlert:[MWMAlert disabledLocationAlert]];
}

- (void)presentLocationServicesDisabledAlert;
{
  [self displayAlert:[MWMAlert locationServicesDisabledAlert]];
}

- (void)presentAlert:(routing::RouterResultCode)type
{
  [self displayAlert:[MWMAlert alert:type]];
}

- (void)presentDownloaderNoConnectionAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock
                                          cancelBlock:(nonnull MWMVoidBlock)cancelBlock
{
  [self displayAlert:[MWMAlert downloaderNoConnectionAlertWithOkBlock:okBlock cancelBlock:cancelBlock]];
}

- (void)presentDownloaderNotEnoughSpaceAlert
{
  [self displayAlert:[MWMAlert downloaderNotEnoughSpaceAlert]];
}

- (void)presentDownloaderInternalErrorAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock
                                           cancelBlock:(nonnull MWMVoidBlock)cancelBlock
{
  [self displayAlert:[MWMAlert downloaderInternalErrorAlertWithOkBlock:okBlock cancelBlock:cancelBlock]];
}

- (void)presentPlaceDoesntExistAlertWithBlock:(MWMStringBlock)block
{
  [self displayAlert:[MWMAlert placeDoesntExistAlertWithBlock:block]];
}

- (void)presentResetChangesAlertWithBlock:(MWMVoidBlock)block
{
  [self displayAlert:[MWMAlert resetChangesAlertWithBlock:block]];
}

- (void)presentDeleteFeatureAlertWithBlock:(MWMVoidBlock)block
{
  [self displayAlert:[MWMAlert deleteFeatureAlertWithBlock:block]];
}

- (void)presentPersonalInfoWarningAlertWithBlock:(nonnull MWMVoidBlock)block
{
  [self displayAlert:[MWMAlert personalInfoWarningAlertWithBlock:block]];
}

- (void)presentTrackWarningAlertWithCancelBlock:(nonnull MWMVoidBlock)block
{
  [self displayAlert:[MWMAlert trackWarningAlertWithCancelBlock:block]];
}

- (void)presentMobileInternetAlertWithBlock:(nonnull MWMMobileInternetAlertCompletionBlock)block
{
  [self displayAlert:[MWMMobileInternetAlert alertWithBlock:block]];
}

- (void)presentInfoAlert:(nonnull NSString *)title text:(nonnull NSString *)text
{
  [self displayAlert:[MWMAlert infoAlert:title text:text]];
}

- (void)presentInfoAlert:(nonnull NSString *)title
{
  [self displayAlert:[MWMAlert infoAlert:title text:nil]];
}

- (void)presentEditorViralAlert
{
  [self displayAlert:[MWMAlert editorViralAlert]];
}
- (void)presentOsmAuthAlert
{
  [self displayAlert:[MWMAlert osmAuthAlert]];
}

- (void)presentOsmReauthAlert
{
  [self displayAlert:[MWMAlert osmReauthAlert]];
}

- (void)presentCreateBookmarkCategoryAlertWithMaxCharacterNum:(NSUInteger)max
                                              minCharacterNum:(NSUInteger)min
                                                     callback:(nonnull MWMCheckStringBlock)callback
{
  auto alert =
      static_cast<MWMBCCreateCategoryAlert *>([MWMAlert createBookmarkCategoryAlertWithMaxCharacterNum:max
                                                                                       minCharacterNum:min
                                                                                              callback:callback]);
  [self displayAlert:alert];
  dispatch_async(dispatch_get_main_queue(), ^{ [alert.textField becomeFirstResponder]; });
}

- (void)presentSpinnerAlertWithTitle:(nonnull NSString *)title cancel:(nullable MWMVoidBlock)cancel
{
  [self displayAlert:[MWMAlert spinnerAlertWithTitle:title cancel:cancel]];
}

- (void)presentBookmarkConversionErrorAlert
{
  [self displayAlert:[MWMAlert bookmarkConversionErrorAlert]];
}

- (void)presentTagsLoadingErrorAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock
                                    cancelBlock:(nonnull MWMVoidBlock)cancelBlock
{
  [self displayAlert:[MWMAlert tagsLoadingErrorAlertWithOkBlock:okBlock cancelBlock:cancelBlock]];
}

- (void)presentBugReportAlertWithTitle:(nonnull NSString *)title
{
  [self displayAlert:[MWMAlert bugReportAlertWithTitle:title]];
}

- (void)presentDefaultAlertWithTitle:(nonnull NSString *)title
                             message:(nullable NSString *)message
                    rightButtonTitle:(nonnull NSString *)rightButtonTitle
                     leftButtonTitle:(nullable NSString *)leftButtonTitle
                   rightButtonAction:(nullable MWMVoidBlock)action
{
  [self displayAlert:[MWMAlert defaultAlertWithTitle:title
                                             message:message
                                    rightButtonTitle:rightButtonTitle
                                     leftButtonTitle:leftButtonTitle
                                   rightButtonAction:action]];
}

- (void)displayAlert:(MWMAlert *)alert
{
  UIViewController * ownerVC = self.ownerViewController;
  BOOL isOwnerLoaded = ownerVC.isViewLoaded;
  if (!isOwnerLoaded)
    return;

  // TODO(igrechuhin): Remove this check on location manager refactoring.
  // Workaround for current location manager duplicate error alerts.
  if ([alert isKindOfClass:[MWMLocationAlert class]])
  {
    for (MWMAlert * view in self.view.subviews)
      if ([view isKindOfClass:[MWMLocationAlert class]])
        return;
  }
  [UIView animateWithDuration:kDefaultAnimationDuration
                        delay:0
                      options:UIViewAnimationOptionBeginFromCurrentState
                   animations:^{
                     for (MWMAlert * view in self.view.subviews)
                       if (view != alert)
                         view.alpha = 0.0;
                   }
                   completion:nil];

  [self removeFromParentViewController];
  alert.alertController = self;
  [ownerVC addChildViewController:self];
  alert.alpha = 0.;
  CGFloat const scale = 1.1;
  alert.transform = CGAffineTransformMakeScale(scale, scale);
  [UIView animateWithDuration:kDefaultAnimationDuration
                   animations:^{
                     self.view.alpha = 1.;
                     alert.alpha = 1.;
                     alert.transform = CGAffineTransformIdentity;
                   }];
  [[MapsAppDelegate theApp].window endEditing:YES];
}

- (void)closeAlert:(nullable MWMVoidBlock)completion
{
  NSArray * subviews = self.view.subviews;
  MWMAlert * closeAlert = subviews.lastObject;
  MWMAlert * showAlert = (subviews.count >= 2 ? subviews[subviews.count - 2] : nil);
  [UIView animateWithDuration:kDefaultAnimationDuration
      delay:0
      options:UIViewAnimationOptionBeginFromCurrentState
      animations:^{
        closeAlert.alpha = 0.;
        if (showAlert)
          showAlert.alpha = 1.;
        else
          self.view.alpha = 0.;
      }
      completion:^(BOOL finished) {
        [closeAlert removeFromSuperview];
        if (!showAlert)
        {
          [self.view removeFromSuperview];
          [self removeFromParentViewController];
        }
        if (completion)
          completion();
      }];
}

@end
