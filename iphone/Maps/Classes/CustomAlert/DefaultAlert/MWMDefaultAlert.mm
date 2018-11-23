#import "MWMDefaultAlert.h"
#import "Statistics.h"

#include "Framework.h"

static CGFloat const kDividerTopConstant = -8.;

@interface MWMDefaultAlert ()

@property(weak, nonatomic) IBOutlet UILabel * messageLabel;
@property(weak, nonatomic) IBOutlet UIButton * rightButton;
@property(weak, nonatomic) IBOutlet UIButton * leftButton;
@property(weak, nonatomic) IBOutlet UILabel * titleLabel;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * rightButtonWidth;
@property(copy, nonatomic) MWMVoidBlock leftButtonAction;
@property(copy, nonatomic, readwrite) MWMVoidBlock rightButtonAction;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * dividerTop;
@property(weak, nonatomic) IBOutlet UIView * vDivider;

@property(copy, nonatomic) NSString * statisticsEvent;

@end

static NSString * const kDefaultAlertNibName = @"MWMDefaultAlert";

@implementation MWMDefaultAlert

+ (instancetype)routeFileNotExistAlert
{
  return [self defaultAlertWithTitle:L(@"dialog_routing_download_files")
                             message:L(@"dialog_routing_download_and_update_all")
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil
                     statisticsEvent:@"Route File Not Exist Alert"];
}

+ (instancetype)routeNotFoundAlert
{
  return [self defaultAlertWithTitle:L(@"dialog_routing_unable_locate_route")
                             message:L(@"dialog_routing_change_start_or_end")
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil
                     statisticsEvent:@"Route File Not Exist Alert"];
}

+ (instancetype)routeNotFoundNoPublicTransportAlert
{
  return [self defaultAlertWithTitle:L(@"transit_not_found")
                             message:nil
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil
                     statisticsEvent:@"transit_not_found"];
}

+ (instancetype)routeNotFoundTooLongPedestrianAlert
{
  return [self defaultAlertWithTitle:L(@"dialog_pedestrian_route_is_long")
                             message:nil
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil
                     statisticsEvent:@"Long Pedestrian Route Alert"];
}

+ (instancetype)locationServiceNotSupportedAlert
{
  return [self defaultAlertWithTitle:L(@"current_location_unknown_error_title")
                             message:L(@"current_location_unknown_error_message")
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil
                     statisticsEvent:@"Location Service Not Supported Alert"];
}

+ (instancetype)noConnectionAlert
{
  MWMDefaultAlert * alert =
      [self defaultAlertWithTitle:L(@"common_check_internet_connection_dialog")
                          message:nil
                 rightButtonTitle:L(@"ok")
                  leftButtonTitle:nil
                rightButtonAction:nil
                  statisticsEvent:@"No Connection Alert"];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)migrationProhibitedAlert
{
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"no_migration_during_navigation")
                                                message:nil
                                       rightButtonTitle:L(@"ok")
                                        leftButtonTitle:nil
                                      rightButtonAction:nil
                                        statisticsEvent:@"Migration Prohibited Alert"];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)deleteMapProhibitedAlert
{
  MWMDefaultAlert * alert =
      [self defaultAlertWithTitle:L(@"downloader_delete_map")
                          message:L(@"downloader_delete_map_while_routing_dialog")
                 rightButtonTitle:L(@"ok")
                  leftButtonTitle:nil
                rightButtonAction:nil
                  statisticsEvent:@"Delete Map Prohibited Alert"];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)unsavedEditsAlertWithOkBlock:(MWMVoidBlock)okBlock
{
  return [self defaultAlertWithTitle:L(@"please_note")
                             message:L(@"downloader_delete_map_dialog")
                    rightButtonTitle:L(@"delete")
                     leftButtonTitle:L(@"cancel")
                   rightButtonAction:okBlock
                     statisticsEvent:@"Editor unsaved changes on delete"];
}

+ (instancetype)noWiFiAlertWithOkBlock:(MWMVoidBlock)okBlock
{
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"download_over_mobile_header")
                                                message:L(@"download_over_mobile_message")
                                       rightButtonTitle:L(@"use_cellular_data")
                                        leftButtonTitle:L(@"cancel")
                                      rightButtonAction:okBlock
                                        statisticsEvent:@"No WiFi Alert"];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)endPointNotFoundAlert
{
  NSString * message =
      [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_end_not_determined"),
                                 L(@"dialog_routing_select_closer_end")];
  return [self defaultAlertWithTitle:L(@"dialog_routing_change_end")
                             message:message
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil
                     statisticsEvent:@"End Point Not Found Alert"];
}

+ (instancetype)startPointNotFoundAlert
{
  NSString * message =
      [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_start_not_determined"),
                                 L(@"dialog_routing_select_closer_start")];
  return [self defaultAlertWithTitle:L(@"dialog_routing_change_start")
                             message:message
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil
                     statisticsEvent:@"Start Point Not Found Alert"];
}

+ (instancetype)intermediatePointNotFoundAlert
{
  return [self defaultAlertWithTitle:L(@"dialog_routing_change_intermediate")
                             message:L(@"dialog_routing_intermediate_not_determined")
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil
                     statisticsEvent:@"Intermediate Point Not Found Alert"];
}

+ (instancetype)internalRoutingErrorAlert
{
  NSString * message =
      [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_application_error"),
                                 L(@"dialog_routing_try_again")];
  return [self defaultAlertWithTitle:L(@"dialog_routing_system_error")
                             message:message
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil
                     statisticsEvent:@"Internal Routing Error Alert"];
}

+ (instancetype)incorrectFeaturePositionAlert
{
  return [self defaultAlertWithTitle:L(@"dialog_incorrect_feature_position")
                             message:L(@"message_invalid_feature_position")
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil
                     statisticsEvent:@"Incorrect Feature Possition Alert"];
}

+ (instancetype)internalErrorAlert
{
  return [self defaultAlertWithTitle:L(@"dialog_routing_system_error")
                             message:L(@"error_system_message")
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil
                     statisticsEvent:@"Internal Error Alert"];
}

+ (instancetype)notEnoughSpaceAlert
{
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"downloader_no_space_title")
                                                message:L(@"migration_no_space_message")
                                       rightButtonTitle:L(@"ok")
                                        leftButtonTitle:nil
                                      rightButtonAction:nil
                                        statisticsEvent:@"Not Enough Space Alert"];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)invalidUserNameOrPasswordAlert
{
  return [self defaultAlertWithTitle:L(@"invalid_username_or_password")
                             message:nil
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil
                     statisticsEvent:@"Invalid User Name or Password Alert"];
}

+ (instancetype)noCurrentPositionAlert
{
  NSString * message =
      [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_error_location_not_found"),
                                 L(@"dialog_routing_location_turn_wifi")];
  return [self defaultAlertWithTitle:L(@"dialog_routing_check_gps")
                             message:message
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil
                     statisticsEvent:@"No Current Position Alert"];
}

+ (instancetype)disabledLocationAlert
{
  MWMVoidBlock action = ^{
    GetFramework().SwitchMyPositionNextMode();
  };
  return [self defaultAlertWithTitle:L(@"dialog_routing_location_turn_on")
                                        message:L(@"dialog_routing_location_unknown_turn_on")
                               rightButtonTitle:L(@"turn_on")
                                leftButtonTitle:L(@"later")
                              rightButtonAction:action
                                statisticsEvent:@"Disabled Location Alert"];
}

+ (instancetype)pointsInDifferentMWMAlert
{
  return [self defaultAlertWithTitle:L(@"routing_failed_cross_mwm_building")
                             message:nil
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil
                     statisticsEvent:@"Points In Different MWM Alert"];
}

+ (instancetype)point2PointAlertWithOkBlock:(MWMVoidBlock)okBlock needToRebuild:(BOOL)needToRebuild
{
  if (needToRebuild)
  {
    return [self defaultAlertWithTitle:L(@"p2p_only_from_current")
                               message:L(@"p2p_reroute_from_current")
                      rightButtonTitle:L(@"ok")
                       leftButtonTitle:L(@"cancel")
                     rightButtonAction:okBlock
                       statisticsEvent:@"Default Alert"];
  }
  else
  {
    return [self defaultAlertWithTitle:L(@"p2p_only_from_current")
                               message:nil
                      rightButtonTitle:L(@"ok")
                       leftButtonTitle:nil
                     rightButtonAction:nil
                       statisticsEvent:@"Default Alert"];
  }
}

+ (instancetype)disableAutoDownloadAlertWithOkBlock:(MWMVoidBlock)okBlock
{
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"disable_auto_download")
                                                message:nil
                                       rightButtonTitle:L(@"_disable")
                                        leftButtonTitle:L(@"cancel")
                                      rightButtonAction:okBlock
                                        statisticsEvent:@"Disable Auto Download Alert"];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)downloaderNoConnectionAlertWithOkBlock:(MWMVoidBlock)okBlock
                                           cancelBlock:(MWMVoidBlock)cancelBlock
{
  MWMDefaultAlert * alert =
      [self defaultAlertWithTitle:L(@"downloader_status_failed")
                          message:L(@"common_check_internet_connection_dialog")
                 rightButtonTitle:L(@"downloader_retry")
                  leftButtonTitle:L(@"cancel")
                rightButtonAction:okBlock
                  statisticsEvent:@"Downloader No Connection Alert"];
  alert.leftButtonAction = cancelBlock;
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)downloaderNotEnoughSpaceAlert
{
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"downloader_no_space_title")
                                                message:L(@"downloader_no_space_message")
                                       rightButtonTitle:L(@"close")
                                        leftButtonTitle:nil
                                      rightButtonAction:nil
                                        statisticsEvent:@"Downloader Not Enough Space Alert"];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)downloaderInternalErrorAlertWithOkBlock:(MWMVoidBlock)okBlock
                                            cancelBlock:(MWMVoidBlock)cancelBlock
{
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"migration_download_error_dialog")
                                                message:nil
                                       rightButtonTitle:L(@"downloader_retry")
                                        leftButtonTitle:L(@"cancel")
                                      rightButtonAction:okBlock
                                        statisticsEvent:@"Downloader Internal Error Alert"];
  alert.leftButtonAction = cancelBlock;
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)downloaderNeedUpdateAlertWithOkBlock:(MWMVoidBlock)okBlock
{
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"downloader_need_update_title")
                                                message:L(@"downloader_need_update_message")
                                       rightButtonTitle:L(@"downloader_status_outdated")
                                        leftButtonTitle:L(@"not_now")
                                      rightButtonAction:okBlock
                                        statisticsEvent:@"Downloader Need Update Alert"];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)routingMigrationAlertWithOkBlock:(MWMVoidBlock)okBlock
{
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"downloader_update_maps")
                                                message:L(@"downloader_mwm_migration_dialog")
                                       rightButtonTitle:L(@"ok")
                                        leftButtonTitle:L(@"cancel")
                                      rightButtonAction:okBlock
                                        statisticsEvent:@"Routing Need Migration Alert"];
  return alert;
}

+ (instancetype)resetChangesAlertWithBlock:(MWMVoidBlock)block
{
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"editor_reset_edits_message")
                                                message:nil
                                       rightButtonTitle:L(@"editor_reset_edits_button")
                                        leftButtonTitle:L(@"cancel")
                                      rightButtonAction:block
                                        statisticsEvent:@"Reset changes alert"];
  return alert;
}

+ (instancetype)deleteFeatureAlertWithBlock:(MWMVoidBlock)block
{
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"editor_remove_place_message")
                                                message:nil
                                       rightButtonTitle:L(@"editor_remove_place_button")
                                        leftButtonTitle:L(@"cancel")
                                      rightButtonAction:block
                                        statisticsEvent:@"Delete feature alert"];
  return alert;
}

+ (instancetype)personalInfoWarningAlertWithBlock:(MWMVoidBlock)block
{
  NSString * message = [NSString stringWithFormat:@"%@\n%@", L(@"editor_share_to_all_dialog_message_1"), L(@"editor_share_to_all_dialog_message_2")];
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"editor_share_to_all_dialog_title")
                                                message:message
                                       rightButtonTitle:L(@"editor_report_problem_send_button")
                                        leftButtonTitle:L(@"cancel")
                                      rightButtonAction:block
                                        statisticsEvent:@"Personal info warning alert"];
  return alert;
}

+ (instancetype)trackWarningAlertWithCancelBlock:(MWMVoidBlock)block
{
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:L(@"recent_track_background_dialog_title")
                                                message:L(@"recent_track_background_dialog_message")
                                       rightButtonTitle:L(@"off_recent_track_background_button")
                                        leftButtonTitle:L(@"continue_download")
                                      rightButtonAction:block
                                        statisticsEvent:@"Track warning alert"];
  return alert;
}

+ (instancetype)infoAlert:(NSString *)title text:(NSString *)text
{
  return [self defaultAlertWithTitle:title
                             message:text
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil
                     statisticsEvent:@"Info Alert"];
}

+ (instancetype)convertBookmarksWithCount:(NSUInteger)count okBlock:(MWMVoidBlock)okBlock
{
  return [self defaultAlertWithTitle:L(@"bookmarks_detect_title")
                             message:[NSString stringWithFormat:L(@"bookmarks_detect_message"), count]
                    rightButtonTitle:L(@"button_convert")
                     leftButtonTitle:L(@"cancel")
                   rightButtonAction:okBlock
                     statisticsEvent:nil];
}

+ (instancetype)restoreBookmarkAlertWithMessage:(NSString *)message
                              rightButtonAction:(MWMVoidBlock)rightButton
                               leftButtonAction:(MWMVoidBlock)leftButton
{
  MWMDefaultAlert * alert =
  [NSBundle.mainBundle loadNibNamed:kDefaultAlertNibName owner:self options:nil].firstObject;
  alert.titleLabel.text = L(@"bookmarks_restore_title");
  alert.messageLabel.text = message;
  [alert.rightButton setTitle:L(@"restore") forState:UIControlStateNormal];
  [alert.leftButton setTitle:L(@"cancel") forState:UIControlStateNormal];
  alert.leftButtonAction = leftButton;
  alert.rightButtonAction = rightButton;
  return alert;
}

+ (instancetype)bookmarkConversionErrorAlert
{
  return [self defaultAlertWithTitle:L(@"bookmarks_convert_error_title")
                             message:L(@"bookmarks_convert_error_message")
                    rightButtonTitle:L(@"ok")
                     leftButtonTitle:nil
                   rightButtonAction:nil
                     statisticsEvent:nil];
}

+ (instancetype)tagsLoadingErrorAlertWithOkBlock:(MWMVoidBlock)okBlock cancelBlock:(MWMVoidBlock)cancelBlock
{
  MWMDefaultAlert * alert =
  [self defaultAlertWithTitle:L(@"title_error_downloading_bookmarks")
                      message:L(@"tags_loading_error_subtitle")
             rightButtonTitle:L(@"downloader_retry")
              leftButtonTitle:L(@"cancel")
            rightButtonAction:okBlock
              statisticsEvent:nil];
  alert.leftButtonAction = cancelBlock;
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)defaultAlertWithTitle:(NSString *)title
                              message:(NSString *)message
                     rightButtonTitle:(NSString *)rightButtonTitle
                      leftButtonTitle:(NSString *)leftButtonTitle
                    rightButtonAction:(MWMVoidBlock)action
                      statisticsEvent:(NSString *)statisticsEvent
{
  if (statisticsEvent)
    [Statistics logEvent:statisticsEvent withParameters:@{kStatAction : kStatOpen}];
  MWMDefaultAlert * alert =
      [NSBundle.mainBundle loadNibNamed:kDefaultAlertNibName owner:self options:nil].firstObject;
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
    alert.vDivider.hidden = YES;
    alert.leftButton.hidden = YES;
    alert.rightButtonWidth.constant = [alert.subviews.firstObject width];
  }
  alert.statisticsEvent = statisticsEvent;
  return alert;
}

#pragma mark - Actions

- (IBAction)rightButtonTap
{
  if (self.statisticsEvent)
    [Statistics logEvent:self.statisticsEvent withParameters:@{kStatAction : kStatApply}];

  [self close:self.rightButtonAction];
}

- (IBAction)leftButtonTap
{
  if (self.statisticsEvent)
    [Statistics logEvent:self.statisticsEvent withParameters:@{kStatAction : kStatClose}];

  [self close:self.leftButtonAction];
}

@end
