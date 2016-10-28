#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMAlertViewController.h"
#import "MWMDefaultAlert.h"
#import "MWMPlacePageViewManager.h"
#import "Statistics.h"
#import "UIButton+RuntimeAttributes.h"
#import "UILabel+RuntimeAttributes.h"

#include "Framework.h"

static CGFloat const kDividerTopConstant = -8.;
static NSString * kStatisticsEvent = @"Default Alert";

@interface MWMDefaultAlert ()

@property (weak, nonatomic) IBOutlet UILabel * messageLabel;
@property (weak, nonatomic) IBOutlet UIButton * rightButton;
@property (weak, nonatomic) IBOutlet UIButton * leftButton;
@property (weak, nonatomic) IBOutlet UILabel * titleLabel;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * rightButtonWidth;
@property (copy, nonatomic) TMWMVoidBlock rightButtonAction;
@property (copy, nonatomic) TMWMVoidBlock leftButtonAction;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint *dividerTop;

@end

static NSString * const kDefaultAlertNibName = @"MWMDefaultAlert";

@implementation MWMDefaultAlert

+ (instancetype)routeFileNotExistAlert
{
  kStatisticsEvent = @"Route File Not Exist Alert";
  return [self defaultAlertWithTitle:L(@"dialog_routing_download_files")
                             message:L(@"dialog_routing_download_and_update_all")
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)routeNotFoundAlert
{
  kStatisticsEvent = @"Route File Not Exist Alert";
  NSString * message = L(@"dialog_routing_change_start_or_end");
  return [self defaultAlertWithTitle:L(@"dialog_routing_unable_locate_route")
                             message:message
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)locationServiceNotSupportedAlert
{
  kStatisticsEvent = @"Location Service Not Supported Alert";
  return [self defaultAlertWithTitle:L(@"current_location_unknown_error_title")
                             message:L(@"current_location_unknown_error_message")
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)locationNotFoundAlertWithOkBlock:(TMWMVoidBlock)okBlock
{
  kStatisticsEvent = @"Location Not Found Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"current_location_unknown_title")
                                                message:L(@"current_location_unknown_message")
                                       rightButtonTitle:L(@"current_location_unknown_continue_button")
                                        leftButtonTitle:L(@"current_location_unknown_stop_button")
                                      rightButtonAction:okBlock];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)noConnectionAlert
{
  kStatisticsEvent = @"No Connection Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"common_check_internet_connection_dialog")
                                                message:nil
                                       rightButtonTitle:L(@"ok")
                                        leftButtonTitle:nil
                                      rightButtonAction:nil];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)migrationProhibitedAlert
{
  kStatisticsEvent = @"Migration Prohibited Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"no_migration_during_navigation")
                                                message:nil
                                       rightButtonTitle:L(@"ok")
                                        leftButtonTitle:nil
                                      rightButtonAction:nil];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)deleteMapProhibitedAlert
{
  kStatisticsEvent = @"Delete Map Prohibited Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"downloader_delete_map")
                                                message:L(@"downloader_delete_map_while_routing_dialog")
                                       rightButtonTitle:L(@"ok")
                                        leftButtonTitle:nil
                                      rightButtonAction:nil];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)unsavedEditsAlertWithOkBlock:(TMWMVoidBlock)okBlock
{
  kStatisticsEvent = @"Editor unsaved changes on delete";
  return [self defaultAlertWithTitle:L(@"please_note")
                             message:L(@"downloader_delete_map_dialog")
                    rightButtonTitle:L(@"delete")
                     leftButtonTitle:L(@"cancel")
                   rightButtonAction:okBlock];
}

+ (instancetype)noWiFiAlertWithOkBlock:(TMWMVoidBlock)okBlock
{
  kStatisticsEvent = @"No WiFi Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"download_over_mobile_header")
                                                message:L(@"download_over_mobile_message")
                                       rightButtonTitle:L(@"use_cellular_data")
                                        leftButtonTitle:L(@"cancel")
                                      rightButtonAction:okBlock];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)endPointNotFoundAlert
{
  kStatisticsEvent = @"End Point Not Found Alert";
  NSString * message =
      [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_end_not_determined"),
                                 L(@"dialog_routing_select_closer_end")];
  return [self defaultAlertWithTitle:L(@"dialog_routing_change_end")
                             message:message
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)startPointNotFoundAlert
{
  kStatisticsEvent = @"Start Point Not Found Alert";
  NSString * message =
      [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_start_not_determined"),
                                 L(@"dialog_routing_select_closer_start")];
  return [self defaultAlertWithTitle:L(@"dialog_routing_change_start")
                             message:message
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)internalRoutingErrorAlert
{
  kStatisticsEvent = @"Internal Routing Error Alert";
  NSString * message =
      [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_application_error"),
                                 L(@"dialog_routing_try_again")];
  return [self defaultAlertWithTitle:L(@"dialog_routing_system_error")
                             message:message
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)incorrectFeauturePositionAlert
{
  kStatisticsEvent = @"Incorrect Feature Possition Alert";
  return [self defaultAlertWithTitle:L(@"dialog_incorrect_feature_position")
                             message:L(@"message_invalid_feature_position")
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)internalErrorAlert
{
  kStatisticsEvent = @"Internal Error Alert";
  return [self defaultAlertWithTitle:L(@"dialog_routing_system_error")
                             message:nil
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)notEnoughSpaceAlert
{
  kStatisticsEvent = @"Not Enough Space Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"migration_download_error_dialog")
                                                message:L(@"migration_no_space_message")
                                       rightButtonTitle:L(@"ok")
                                        leftButtonTitle:nil
                                      rightButtonAction:nil];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)invalidUserNameOrPasswordAlert
{
  kStatisticsEvent = @"Invalid User Name or Password Alert";
  return [self defaultAlertWithTitle:L(@"invalid_username_or_password")
                             message:nil
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)noCurrentPositionAlert
{
  kStatisticsEvent = @"No Current Position Alert";
  NSString * message =
      [NSString stringWithFormat:@"%@\n\n%@", L(@"common_current_location_unknown_dialog"),
                                 L(@"dialog_routing_location_turn_wifi")];
  return [self defaultAlertWithTitle:L(@"dialog_routing_check_gps")
                             message:message
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)disabledLocationAlert
{
  kStatisticsEvent = @"Disabled Location Alert";
  TMWMVoidBlock action = ^{
    GetFramework().SwitchMyPositionNextMode();
  };
  return [MWMDefaultAlert defaultAlertWithTitle:L(@"dialog_routing_location_turn_on")
                                        message:L(@"dialog_routing_location_unknown_turn_on")
                               rightButtonTitle:L(@"turn_on")
                                leftButtonTitle:L(@"later")
                              rightButtonAction:action];
}

+ (instancetype)pointsInDifferentMWMAlert
{
  kStatisticsEvent = @"Points In Different MWM Alert";
  return [self defaultAlertWithTitle:L(@"routing_failed_cross_mwm_building")
                             message:nil
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)point2PointAlertWithOkBlock:(TMWMVoidBlock)okBlock needToRebuild:(BOOL)needToRebuild
{
  if (needToRebuild)
  {
    return [self defaultAlertWithTitle:L(@"p2p_only_from_current")
                               message:L(@"p2p_reroute_from_current")
                      rightButtonTitle:L(@"ok")
                       leftButtonTitle:L(@"cancel")
                     rightButtonAction:okBlock];
  }
  else
  {
    return [self defaultAlertWithTitle:L(@"p2p_only_from_current")
                               message:nil
                      rightButtonTitle:L(@"ok")
                       leftButtonTitle:nil
                     rightButtonAction:nil];
  }
}

+ (instancetype)disableAutoDownloadAlertWithOkBlock:(TMWMVoidBlock)okBlock
{
  kStatisticsEvent = @"Disable Auto Download Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"disable_auto_download")
                                                message:nil
                                       rightButtonTitle:L(@"_disable")
                                        leftButtonTitle:L(@"cancel")
                                      rightButtonAction:okBlock];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)downloaderNoConnectionAlertWithOkBlock:(TMWMVoidBlock)okBlock cancelBlock:(TMWMVoidBlock)cancelBlock
{
  kStatisticsEvent = @"Downloader No Connection Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"downloader_status_failed")
                                                message:L(@"common_check_internet_connection_dialog")
                                       rightButtonTitle:L(@"downloader_retry")
                                        leftButtonTitle:L(@"cancel")
                                      rightButtonAction:okBlock];
  alert.leftButtonAction = cancelBlock;
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)downloaderNotEnoughSpaceAlert
{
  kStatisticsEvent = @"Downloader Not Enough Space Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"downloader_no_space_title")
                                                message:L(@"downloader_no_space_message")
                                       rightButtonTitle:L(@"close")
                                        leftButtonTitle:nil
                                      rightButtonAction:nil];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)downloaderInternalErrorAlertWithOkBlock:(TMWMVoidBlock)okBlock cancelBlock:(TMWMVoidBlock)cancelBlock
{
  kStatisticsEvent = @"Downloader Internal Error Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"migration_download_error_dialog")
                                                message:nil
                                       rightButtonTitle:L(@"downloader_retry")
                                        leftButtonTitle:L(@"cancel")
                                      rightButtonAction:okBlock];
  alert.leftButtonAction = cancelBlock;
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)downloaderNeedUpdateAlertWithOkBlock:(TMWMVoidBlock)okBlock
{
  kStatisticsEvent = @"Downloader Need Update Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"downloader_need_update_title")
                                                message:L(@"downloader_need_update_message")
                                       rightButtonTitle:L(@"downloader_status_outdated")
                                        leftButtonTitle:L(@"not_now")
                                      rightButtonAction:okBlock];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)routingMigrationAlertWithOkBlock:(TMWMVoidBlock)okBlock
{
  kStatisticsEvent = @"Routing Need Migration Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"downloader_update_maps")
                                                message:L(@"downloader_mwm_migration_dialog")
                                       rightButtonTitle:L(@"ok")
                                        leftButtonTitle:L(@"cancel")
                                      rightButtonAction:okBlock];
  return alert;
}

+ (instancetype)resetChangesAlertWithBlock:(TMWMVoidBlock)block
{
  kStatisticsEvent = @"Reset changes alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"editor_reset_edits_message")
                                                message:nil
                                       rightButtonTitle:L(@"editor_reset_edits_button")
                                        leftButtonTitle:L(@"cancel")
                                      rightButtonAction:block];
  return alert;
}

+ (instancetype)deleteFeatureAlertWithBlock:(TMWMVoidBlock)block
{
  kStatisticsEvent = @"Delete feature alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"editor_remove_place_message")
                                                message:nil
                                       rightButtonTitle:L(@"editor_remove_place_button")
                                        leftButtonTitle:L(@"cancel")
                                      rightButtonAction:block];
  return alert;
}

+ (instancetype)personalInfoWarningAlertWithBlock:(TMWMVoidBlock)block
{
  kStatisticsEvent = @"Personal info warning alert";
  NSString * message = [NSString stringWithFormat:@"%@\n%@", L(@"editor_share_to_all_dialog_message_1"), L(@"editor_share_to_all_dialog_message_2")];
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"editor_share_to_all_dialog_title")
                                                message:message
                                       rightButtonTitle:L(@"editor_report_problem_send_button")
                                        leftButtonTitle:L(@"cancel")
                                      rightButtonAction:block];
  return alert;
}

+ (instancetype)trackWarningAlertWithCancelBlock:(TMWMVoidBlock)block
{
  kStatisticsEvent = @"Track warning alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"recent_track_background_dialog_title")
                                                message:L(@"recent_track_background_dialog_message")
                                       rightButtonTitle:L(@"off_recent_track_background_button")
                                        leftButtonTitle:L(@"continue_download")
                                      rightButtonAction:block];
  return alert;
}

+ (instancetype)defaultAlertWithTitle:(nonnull NSString *)title
                              message:(nullable NSString *)message
                     rightButtonTitle:(nonnull NSString *)rightButtonTitle
                      leftButtonTitle:(nullable NSString *)leftButtonTitle
                    rightButtonAction:(nullable TMWMVoidBlock)action
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatOpen}];
  MWMDefaultAlert * alert = [
      [[NSBundle mainBundle] loadNibNamed:kDefaultAlertNibName owner:self options:nil] firstObject];
  alert.titleLabel.text = title;
  alert.messageLabel.text = message;
  if (!message)
  {
    alert.dividerTop.constant = kDividerTopConstant;
    [alert layoutIfNeeded];
  }
  
  [alert.rightButton setTitle:rightButtonTitle forState:UIControlStateNormal];
  [alert.rightButton setTitle:rightButtonTitle forState:UIControlStateDisabled];

  alert.rightButtonAction = action;
  if (leftButtonTitle)
  {
    [alert.leftButton setTitle:leftButtonTitle forState:UIControlStateNormal];
    [alert.leftButton setTitle:leftButtonTitle forState:UIControlStateDisabled];
  }
  else
  {
    alert.leftButton.hidden = YES;
    alert.rightButtonWidth.constant = [alert.subviews.firstObject width];
  }
  return alert;
}

#pragma mark - Actions

- (IBAction)rightButtonTap
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatApply}];
  [self close:self.rightButtonAction];
}

- (IBAction)leftButtonTap
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatClose}];
  [self close:self.leftButtonAction];
}

@end
