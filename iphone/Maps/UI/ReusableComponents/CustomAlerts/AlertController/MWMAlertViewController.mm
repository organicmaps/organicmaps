#import "MWMAlertViewController+CPP.h"
#import "MWMController.h"
#import "MWMDownloadTransitMapAlert.h"
#import "MWMLocationAlert.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "SwiftBridge.h"

#include <CoreApi/Framework.h>

static NSString * const kAlertControllerNibIdentifier = @"MWMAlertViewController";

@interface MWMAlertViewController () <UIGestureRecognizerDelegate>

@property(weak, nonatomic, readwrite) UIViewController * ownerViewController;
@property(weak, nonatomic) UIAlertController * systemAlertController;

- (void)presentSystemAlertWithTitle:(nonnull NSString *)title
                            message:(nullable NSString *)message
                   rightButtonTitle:(nonnull NSString *)rightButtonTitle
                    leftButtonTitle:(nullable NSString *)leftButtonTitle
                  rightButtonAction:(nullable MWMVoidBlock)rightButtonAction
                   leftButtonAction:(nullable MWMVoidBlock)leftButtonAction;

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
  if (needToRebuild)
  {
    [self presentSystemAlertWithTitle:L(@"p2p_only_from_current")
                              message:L(@"p2p_reroute_from_current")
                     rightButtonTitle:L(@"ok")
                      leftButtonTitle:L(@"cancel")
                    rightButtonAction:okBlock
                     leftButtonAction:nil];
  }
  else
  {
    [self presentSystemAlertWithTitle:L(@"p2p_only_from_current")
                              message:nil
                     rightButtonTitle:L(@"ok")
                      leftButtonTitle:nil
                    rightButtonAction:nil
                     leftButtonAction:nil];
  }
}

- (void)presentLocationServiceNotSupportedAlert
{
  [self presentSystemAlertWithTitle:L(@"current_location_unknown_error_title")
                            message:L(@"current_location_unknown_error_message")
                   rightButtonTitle:L(@"ok")
                    leftButtonTitle:nil
                  rightButtonAction:nil
                   leftButtonAction:nil];
}
- (void)presentNoConnectionAlert
{
  [self presentSystemAlertWithTitle:L(@"common_check_internet_connection_dialog")
                            message:nil
                   rightButtonTitle:L(@"ok")
                    leftButtonTitle:nil
                  rightButtonAction:nil
                   leftButtonAction:nil];
}

- (void)presentDeleteMapProhibitedAlert
{
  [self presentSystemAlertWithTitle:L(@"downloader_delete_map")
                            message:L(@"downloader_delete_map_while_routing_dialog")
                   rightButtonTitle:L(@"ok")
                    leftButtonTitle:nil
                  rightButtonAction:nil
                   leftButtonAction:nil];
}
- (void)presentUnsavedEditsAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock
{
  [self presentSystemAlertWithTitle:L(@"please_note")
                            message:L(@"downloader_delete_map_dialog")
                   rightButtonTitle:L(@"delete")
                    leftButtonTitle:L(@"cancel")
                  rightButtonAction:okBlock
                   leftButtonAction:nil];
}

- (void)presentNoWiFiAlertWithOkBlock:(nullable MWMVoidBlock)okBlock andCancelBlock:(MWMVoidBlock)cancelBlock
{
  [self presentSystemAlertWithTitle:L(@"download_over_mobile_header")
                            message:L(@"download_over_mobile_message")
                   rightButtonTitle:L(@"use_cellular_data")
                    leftButtonTitle:L(@"cancel")
                  rightButtonAction:okBlock
                   leftButtonAction:cancelBlock];
}

- (void)presentIncorrectFeauturePositionAlert
{
  [self presentSystemAlertWithTitle:L(@"dialog_incorrect_feature_position")
                            message:L(@"message_invalid_feature_position")
                   rightButtonTitle:L(@"ok")
                    leftButtonTitle:nil
                  rightButtonAction:nil
                   leftButtonAction:nil];
}

- (void)presentNotEnoughSpaceAlert
{
  [self presentSystemAlertWithTitle:L(@"downloader_no_space_title")
                            message:L(@"migration_no_space_message")
                   rightButtonTitle:L(@"ok")
                    leftButtonTitle:nil
                  rightButtonAction:nil
                   leftButtonAction:nil];
}
- (void)presentInvalidUserNameOrPasswordAlert
{
  [self presentSystemAlertWithTitle:L(@"invalid_username_or_password")
                            message:nil
                   rightButtonTitle:L(@"ok")
                    leftButtonTitle:nil
                  rightButtonAction:nil
                   leftButtonAction:nil];
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

- (void)presentLocationServicesDisabledAlert;
{
  [self displayAlert:[MWMAlert locationServicesDisabledAlert]];
}

- (void)presentAlert:(routing::RouterResultCode)type
{
  switch (type)
  {
  case routing::RouterResultCode::NoCurrentPosition:
  {
    NSString * message = [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_error_location_not_found"),
                                                    L(@"dialog_routing_location_turn_wifi")];
    [self presentSystemAlertWithTitle:L(@"dialog_routing_check_gps")
                              message:message
                     rightButtonTitle:L(@"ok")
                      leftButtonTitle:nil
                    rightButtonAction:nil
                     leftButtonAction:nil];
    break;
  }
  case routing::RouterResultCode::StartPointNotFound:
  {
    NSString * message = [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_start_not_determined"),
                                                    L(@"dialog_routing_select_closer_start")];
    [self presentSystemAlertWithTitle:L(@"dialog_routing_change_start")
                              message:message
                     rightButtonTitle:L(@"ok")
                      leftButtonTitle:nil
                    rightButtonAction:nil
                     leftButtonAction:nil];
    break;
  }
  case routing::RouterResultCode::EndPointNotFound:
  {
    NSString * message = [NSString
        stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_end_not_determined"), L(@"dialog_routing_select_closer_end")];
    [self presentSystemAlertWithTitle:L(@"dialog_routing_change_end")
                              message:message
                     rightButtonTitle:L(@"ok")
                      leftButtonTitle:nil
                    rightButtonAction:nil
                     leftButtonAction:nil];
    break;
  }
  case routing::RouterResultCode::PointsInDifferentMWM:
    [self presentSystemAlertWithTitle:L(@"routing_failed_cross_mwm_building")
                              message:nil
                     rightButtonTitle:L(@"ok")
                      leftButtonTitle:nil
                    rightButtonAction:nil
                     leftButtonAction:nil];
    break;
  case routing::RouterResultCode::TransitRouteNotFoundNoNetwork:
    [self presentSystemAlertWithTitle:L(@"transit_not_found")
                              message:nil
                     rightButtonTitle:L(@"ok")
                      leftButtonTitle:nil
                    rightButtonAction:nil
                     leftButtonAction:nil];
    break;
  case routing::RouterResultCode::TransitRouteNotFoundTooLongPedestrian:
    [self presentSystemAlertWithTitle:L(@"dialog_pedestrian_route_is_long_header")
                              message:L(@"dialog_pedestrian_route_is_long_message")
                     rightButtonTitle:L(@"ok")
                      leftButtonTitle:nil
                    rightButtonAction:nil
                     leftButtonAction:nil];
    break;
  case routing::RouterResultCode::RouteNotFoundRedressRouteError:
  case routing::RouterResultCode::RouteNotFound:
  case routing::RouterResultCode::InconsistentMWMandRoute:
    [self presentSystemAlertWithTitle:L(@"dialog_routing_unable_locate_route")
                              message:L(@"dialog_routing_change_start_or_end")
                     rightButtonTitle:L(@"ok")
                      leftButtonTitle:nil
                    rightButtonAction:nil
                     leftButtonAction:nil];
    break;
  case routing::RouterResultCode::RouteFileNotExist:
  case routing::RouterResultCode::FileTooOld:
    [self presentSystemAlertWithTitle:L(@"dialog_routing_download_files")
                              message:L(@"dialog_routing_download_and_update_all")
                     rightButtonTitle:L(@"ok")
                      leftButtonTitle:nil
                    rightButtonAction:nil
                     leftButtonAction:nil];
    break;
  case routing::RouterResultCode::InternalError:
  {
    NSString * message =
        [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_application_error"), L(@"dialog_routing_try_again")];
    [self presentSystemAlertWithTitle:L(@"dialog_routing_system_error")
                              message:message
                     rightButtonTitle:L(@"ok")
                      leftButtonTitle:nil
                    rightButtonAction:nil
                     leftButtonAction:nil];
    break;
  }
  case routing::RouterResultCode::IntermediatePointNotFound:
    [self presentSystemAlertWithTitle:L(@"dialog_routing_change_intermediate")
                              message:L(@"dialog_routing_intermediate_not_determined")
                     rightButtonTitle:L(@"ok")
                      leftButtonTitle:nil
                    rightButtonAction:nil
                     leftButtonAction:nil];
    break;
  case routing::RouterResultCode::Cancelled:
  case routing::RouterResultCode::NoError:
  case routing::RouterResultCode::HasWarnings:
  case routing::RouterResultCode::NeedMoreMaps: break;
  }
}

- (void)presentDownloaderNoConnectionAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock
                                          cancelBlock:(nonnull MWMVoidBlock)cancelBlock
{
  [self presentSystemAlertWithTitle:L(@"downloader_status_failed")
                            message:L(@"common_check_internet_connection_dialog")
                   rightButtonTitle:L(@"downloader_retry")
                    leftButtonTitle:L(@"cancel")
                  rightButtonAction:okBlock
                   leftButtonAction:cancelBlock];
}

- (void)presentDownloaderNotEnoughSpaceAlert
{
  [self presentSystemAlertWithTitle:L(@"downloader_no_space_title")
                            message:L(@"downloader_no_space_message")
                   rightButtonTitle:L(@"close")
                    leftButtonTitle:nil
                  rightButtonAction:nil
                   leftButtonAction:nil];
}

- (void)presentDownloaderInternalErrorAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock
                                           cancelBlock:(nonnull MWMVoidBlock)cancelBlock
{
  [self presentSystemAlertWithTitle:L(@"migration_download_error_dialog")
                            message:nil
                   rightButtonTitle:L(@"downloader_retry")
                    leftButtonTitle:L(@"cancel")
                  rightButtonAction:okBlock
                   leftButtonAction:cancelBlock];
}

- (void)presentPlaceDoesntExistAlertWithBlock:(MWMStringBlock)block
{
  [self displayAlert:[MWMAlert placeDoesntExistAlertWithBlock:block]];
}

- (void)presentResetChangesAlertWithBlock:(MWMVoidBlock)block
{
  [self presentSystemAlertWithTitle:L(@"editor_reset_edits_message")
                            message:nil
                   rightButtonTitle:L(@"editor_reset_edits_button")
                    leftButtonTitle:L(@"cancel")
                  rightButtonAction:block
                   leftButtonAction:nil];
}

- (void)presentDeleteFeatureAlertWithBlock:(MWMVoidBlock)block
{
  [self presentSystemAlertWithTitle:L(@"editor_remove_place_message")
                            message:nil
                   rightButtonTitle:L(@"editor_remove_place_button")
                    leftButtonTitle:L(@"cancel")
                  rightButtonAction:block
                   leftButtonAction:nil];
}

- (void)presentPersonalInfoWarningAlertWithBlock:(nonnull MWMVoidBlock)block
{
  NSString * message = [NSString stringWithFormat:@"%@\n%@", L(@"editor_share_to_all_dialog_message_1"),
                                                  L(@"editor_share_to_all_dialog_message_2")];
  [self presentSystemAlertWithTitle:L(@"editor_share_to_all_dialog_title")
                            message:message
                   rightButtonTitle:L(@"editor_report_problem_send_button")
                    leftButtonTitle:L(@"cancel")
                  rightButtonAction:block
                   leftButtonAction:nil];
}

- (void)presentTrackWarningAlertWithCancelBlock:(nonnull MWMVoidBlock)block
{
  [self presentSystemAlertWithTitle:L(@"recent_track_background_dialog_title")
                            message:L(@"recent_track_background_dialog_message")
                   rightButtonTitle:L(@"off_recent_track_background_button")
                    leftButtonTitle:L(@"continue_button")
                  rightButtonAction:block
                   leftButtonAction:nil];
}

- (void)presentMobileInternetAlertWithBlock:(nonnull MWMMobileInternetAlertCompletionBlock)block
{
  [self displayAlert:[MWMMobileInternetAlert alertWithBlock:block]];
}

- (void)presentInfoAlert:(nonnull NSString *)title text:(nonnull NSString *)text
{
  [self presentSystemAlertWithTitle:title
                            message:text
                   rightButtonTitle:L(@"ok")
                    leftButtonTitle:nil
                  rightButtonAction:nil
                   leftButtonAction:nil];
}

- (void)presentInfoAlert:(nonnull NSString *)title
{
  [self presentSystemAlertWithTitle:title
                            message:nil
                   rightButtonTitle:L(@"ok")
                    leftButtonTitle:nil
                  rightButtonAction:nil
                   leftButtonAction:nil];
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
  [self presentSystemAlertWithTitle:L(@"bookmarks_convert_error_title")
                            message:L(@"bookmarks_convert_error_message")
                   rightButtonTitle:L(@"ok")
                    leftButtonTitle:nil
                  rightButtonAction:nil
                   leftButtonAction:nil];
}

- (void)presentTagsLoadingErrorAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock
                                    cancelBlock:(nonnull MWMVoidBlock)cancelBlock
{
  [self presentSystemAlertWithTitle:L(@"title_error_downloading_bookmarks")
                            message:L(@"tags_loading_error_subtitle")
                   rightButtonTitle:L(@"downloader_retry")
                    leftButtonTitle:L(@"cancel")
                  rightButtonAction:okBlock
                   leftButtonAction:cancelBlock];
}

- (void)presentBugReportAlertWithTitle:(nonnull NSString *)title
{
  [self presentSystemAlertWithTitle:title
                            message:L(@"bugreport_alert_message")
                   rightButtonTitle:L(@"report_a_bug")
                    leftButtonTitle:L(@"cancel")
                  rightButtonAction:^{ [MailComposer sendBugReportWithTitle:title]; }
                   leftButtonAction:nil];
}

- (void)presentDefaultAlertWithTitle:(nonnull NSString *)title
                             message:(nullable NSString *)message
                    rightButtonTitle:(nonnull NSString *)rightButtonTitle
                     leftButtonTitle:(nullable NSString *)leftButtonTitle
                   rightButtonAction:(nullable MWMVoidBlock)action
{
  [self presentSystemAlertWithTitle:title
                            message:message
                   rightButtonTitle:rightButtonTitle
                    leftButtonTitle:leftButtonTitle
                  rightButtonAction:action
                   leftButtonAction:nil];
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
  UIAlertController * alert = self.systemAlertController;
  if (alert)
  {
    self.systemAlertController = nil;
    [alert dismissViewControllerAnimated:YES completion:completion];
    return;
  }
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

- (BOOL)isAlertDisplayed
{
  return self.view.superview != nil || self.systemAlertController != nil;
}

- (void)presentSystemAlertWithTitle:(nonnull NSString *)title
                            message:(nullable NSString *)message
                   rightButtonTitle:(nonnull NSString *)rightButtonTitle
                    leftButtonTitle:(nullable NSString *)leftButtonTitle
                  rightButtonAction:(nullable MWMVoidBlock)rightButtonAction
                   leftButtonAction:(nullable MWMVoidBlock)leftButtonAction
{
  if (!NSThread.isMainThread)
  {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self presentSystemAlertWithTitle:title
                                message:message
                       rightButtonTitle:rightButtonTitle
                        leftButtonTitle:leftButtonTitle
                      rightButtonAction:rightButtonAction
                       leftButtonAction:leftButtonAction];
    });
    return;
  }

  UIViewController * ownerVC = self.ownerViewController;
  if (!ownerVC.isViewLoaded || ownerVC.view.window == nil)
    return;

  UIAlertController * existingAlert = self.systemAlertController;
  if (existingAlert)
  {
    self.systemAlertController = nil;
    [existingAlert dismissViewControllerAnimated:NO completion:nil];
  }

  UIAlertController * alert = [UIAlertController alertControllerWithTitle:title
                                                                  message:message
                                                           preferredStyle:UIAlertControllerStyleAlert];

  __weak __typeof(self) weakSelf = self;
  UIAlertAction * rightAction = [UIAlertAction actionWithTitle:rightButtonTitle
                                                         style:UIAlertActionStyleDefault
                                                       handler:^(__unused UIAlertAction * action) {
                                                         __typeof(self) strongSelf = weakSelf;
                                                         strongSelf.systemAlertController = nil;
                                                         if (rightButtonAction)
                                                           rightButtonAction();
                                                       }];
  [alert addAction:rightAction];

  if (leftButtonTitle)
  {
    UIAlertAction * leftAction = [UIAlertAction actionWithTitle:leftButtonTitle
                                                          style:UIAlertActionStyleCancel
                                                        handler:^(__unused UIAlertAction * action) {
                                                          __typeof(self) strongSelf = weakSelf;
                                                          strongSelf.systemAlertController = nil;
                                                          if (leftButtonAction)
                                                            leftButtonAction();
                                                        }];
    [alert addAction:leftAction];
  }

  self.systemAlertController = alert;
  [ownerVC presentViewController:alert animated:YES completion:nil];
}

@end
