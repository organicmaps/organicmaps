#import "LocationManager.h"
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
  return [self defaultAlertWithTitle:@"dialog_routing_download_files"
                             message:@"dialog_routing_download_and_update_all"
                    rightButtonTitle:@"ok"
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)routeNotFoundAlert
{
  kStatisticsEvent = @"Route File Not Exist Alert";
  NSString * message = L(@"dialog_routing_change_start_or_end");
  return [self defaultAlertWithTitle:@"dialog_routing_unable_locate_route"
                             message:message
                    rightButtonTitle:@"ok"
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)locationServiceNotSupportedAlert
{
  kStatisticsEvent = @"Location Service Not Supported Alert";
  return [self defaultAlertWithTitle:@"current_location_unknown_error_title"
                             message:@"current_location_unknown_error_message"
                    rightButtonTitle:@"ok"
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)locationNotFoundAlertWithOkBlock:(TMWMVoidBlock)okBlock
{
  kStatisticsEvent = @"Location Not Found Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"current_location_unknown_title"
                                                message:@"current_location_unknown_message"
                                       rightButtonTitle:@"current_location_unknown_continue_button"
                                        leftButtonTitle:@"current_location_unknown_stop_button"
                                      rightButtonAction:okBlock];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)noConnectionAlert
{
  kStatisticsEvent = @"No Connection Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"common_check_internet_connection_dialog"
                                                message:nil
                                       rightButtonTitle:@"ok"
                                        leftButtonTitle:nil
                                      rightButtonAction:nil];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)migrationProhibitedAlert
{
  kStatisticsEvent = @"Migration Prohibited Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"no_migration_during_navigation"
                                                message:nil
                                       rightButtonTitle:@"ok"
                                        leftButtonTitle:nil
                                      rightButtonAction:nil];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)deleteMapProhibitedAlert
{
  kStatisticsEvent = @"Delete Map Prohibited Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"downloader_delete_map"
                                                message:@"downloader_delete_map_while_routing_dialog"
                                       rightButtonTitle:@"ok"
                                        leftButtonTitle:nil
                                      rightButtonAction:nil];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)unsavedEditsAlertWithOkBlock:(TMWMVoidBlock)okBlock
{
  kStatisticsEvent = @"Editor unsaved changes on delete";
  return [self defaultAlertWithTitle:@"please_note"
                             message:@"downloader_delete_map_dialog"
                    rightButtonTitle:@"delete"
                     leftButtonTitle:@"cancel"
                   rightButtonAction:okBlock];
}

+ (instancetype)noWiFiAlertWithOkBlock:(TMWMVoidBlock)okBlock
{
  kStatisticsEvent = @"No WiFi Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"download_over_mobile_header"
                                                message:@"download_over_mobile_message"
                                       rightButtonTitle:@"use_cellular_data"
                                        leftButtonTitle:@"cancel"
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
  return [self defaultAlertWithTitle:@"dialog_routing_change_end"
                             message:message
                    rightButtonTitle:@"ok"
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)startPointNotFoundAlert
{
  kStatisticsEvent = @"Start Point Not Found Alert";
  NSString * message =
      [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_start_not_determined"),
                                 L(@"dialog_routing_select_closer_start")];
  return [self defaultAlertWithTitle:@"dialog_routing_change_start"
                             message:message
                    rightButtonTitle:@"ok"
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)internalRoutingErrorAlert
{
  kStatisticsEvent = @"Internal Routing Error Alert";
  NSString * message =
      [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_application_error"),
                                 L(@"dialog_routing_try_again")];
  return [self defaultAlertWithTitle:@"dialog_routing_system_error"
                             message:message
                    rightButtonTitle:@"ok"
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)incorrectFeauturePositionAlert
{
  kStatisticsEvent = @"Incorrect Feature Possition Alert";
  return [self defaultAlertWithTitle:@"dialog_incorrect_feature_position"
                             message:@"message_invalid_feature_position"
                    rightButtonTitle:@"ok"
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)internalErrorAlert
{
  kStatisticsEvent = @"Internal Error Alert";
  return [self defaultAlertWithTitle:@"dialog_routing_system_error"
                             message:nil
                    rightButtonTitle:@"ok"
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)notEnoughSpaceAlert
{
  kStatisticsEvent = @"Not Enough Space Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"migration_download_error_dialog"
                                                message:@"migration_no_space_message"
                                       rightButtonTitle:@"ok"
                                        leftButtonTitle:nil
                                      rightButtonAction:nil];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)invalidUserNameOrPasswordAlert
{
  kStatisticsEvent = @"Invalid User Name or Password Alert";
  return [self defaultAlertWithTitle:@"invalid_username_or_password"
                             message:nil
                    rightButtonTitle:@"ok"
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)noCurrentPositionAlert
{
  kStatisticsEvent = @"No Current Position Alert";
  NSString * message =
      [NSString stringWithFormat:@"%@\n\n%@", L(@"common_current_location_unknown_dialog"),
                                 L(@"dialog_routing_location_turn_wifi")];
  return [self defaultAlertWithTitle:@"dialog_routing_check_gps"
                             message:message
                    rightButtonTitle:@"ok"
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)disabledLocationAlert
{
  kStatisticsEvent = @"Disabled Location Alert";
  TMWMVoidBlock action = ^{
    GetFramework().SwitchMyPositionNextMode();
  };
  return [MWMDefaultAlert defaultAlertWithTitle:@"dialog_routing_location_turn_on"
                                        message:@"dialog_routing_location_unknown_turn_on"
                               rightButtonTitle:@"turn_on"
                                leftButtonTitle:@"later"
                              rightButtonAction:action];
}

+ (instancetype)pointsInDifferentMWMAlert
{
  kStatisticsEvent = @"Points In Different MWM Alert";
  return [self defaultAlertWithTitle:@"routing_failed_cross_mwm_building"
                             message:nil
                    rightButtonTitle:@"ok"
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)point2PointAlertWithOkBlock:(TMWMVoidBlock)okBlock needToRebuild:(BOOL)needToRebuild
{
  if (needToRebuild)
  {
    return [self defaultAlertWithTitle:@"p2p_only_from_current"
                               message:@"p2p_reroute_from_current"
                      rightButtonTitle:@"ok"
                       leftButtonTitle:@"cancel"
                     rightButtonAction:okBlock];
  }
  else
  {
    return [self defaultAlertWithTitle:@"p2p_only_from_current"
                               message:nil
                      rightButtonTitle:@"ok"
                       leftButtonTitle:nil
                     rightButtonAction:nil];
  }
}

+ (instancetype)disableAutoDownloadAlertWithOkBlock:(TMWMVoidBlock)okBlock
{
  kStatisticsEvent = @"Disable Auto Download Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"disable_auto_download"
                                                message:nil
                                       rightButtonTitle:@"_disable"
                                        leftButtonTitle:@"cancel"
                                      rightButtonAction:okBlock];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)downloaderNoConnectionAlertWithOkBlock:(TMWMVoidBlock)okBlock cancelBlock:(TMWMVoidBlock)cancelBlock
{
  kStatisticsEvent = @"Downloader No Connection Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"downloader_status_failed"
                                                message:@"common_check_internet_connection_dialog"
                                       rightButtonTitle:@"downloader_retry"
                                        leftButtonTitle:@"cancel"
                                      rightButtonAction:okBlock];
  alert.leftButtonAction = cancelBlock;
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)downloaderNotEnoughSpaceAlert
{
  kStatisticsEvent = @"Downloader Not Enough Space Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"downloader_no_space_title"
                                                message:@"downloader_no_space_message"
                                       rightButtonTitle:@"close"
                                        leftButtonTitle:nil
                                      rightButtonAction:nil];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)downloaderInternalErrorAlertWithOkBlock:(TMWMVoidBlock)okBlock cancelBlock:(TMWMVoidBlock)cancelBlock
{
  kStatisticsEvent = @"Downloader Internal Error Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"migration_download_error_dialog"
                                                message:nil
                                       rightButtonTitle:@"downloader_retry"
                                        leftButtonTitle:@"cancel"
                                      rightButtonAction:okBlock];
  alert.leftButtonAction = cancelBlock;
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)downloaderNeedUpdateAlertWithOkBlock:(TMWMVoidBlock)okBlock
{
  kStatisticsEvent = @"Downloader Need Update Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"downloader_need_update_title"
                                                message:@"downloader_need_update_message"
                                       rightButtonTitle:@"downloader_status_outdated"
                                        leftButtonTitle:@"not_now"
                                      rightButtonAction:okBlock];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)routingMigrationAlertWithOkBlock:(TMWMVoidBlock)okBlock
{
  kStatisticsEvent = @"Routing Need Migration Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"downloader_update_maps"
                                                message:@"downloader_mwm_migration_dialog"
                                       rightButtonTitle:@"ok"
                                        leftButtonTitle:@"cancel"
                                      rightButtonAction:okBlock];
  return alert;
}

+ (instancetype)resetChangesAlertWithBlock:(TMWMVoidBlock)block
{
  kStatisticsEvent = @"Reset changes alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"editor_reset_edits_message"
                                                message:nil
                                       rightButtonTitle:@"editor_reset_edits_button"
                                        leftButtonTitle:@"cancel"
                                      rightButtonAction:block];
  return alert;
}

+ (instancetype)deleteFeatureAlertWithBlock:(TMWMVoidBlock)block
{
  kStatisticsEvent = @"Delete feature alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"editor_remove_place_message"
                                                message:nil
                                       rightButtonTitle:@"editor_remove_place_button"
                                        leftButtonTitle:@"cancel"
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
  alert.titleLabel.localizedText = title;
  alert.messageLabel.localizedText = message;
  if (!message)
  {
    alert.dividerTop.constant = kDividerTopConstant;
    [alert layoutIfNeeded];
  }
  alert.rightButton.localizedText = rightButtonTitle;
  alert.rightButtonAction = action;
  if (leftButtonTitle)
  {
    alert.leftButton.localizedText = leftButtonTitle;
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
  if (self.rightButtonAction)
    self.rightButtonAction();
  [self close];
}

- (IBAction)leftButtonTap
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatClose}];
  if (self.leftButtonAction)
    self.leftButtonAction();
  [self close];
}

@end
