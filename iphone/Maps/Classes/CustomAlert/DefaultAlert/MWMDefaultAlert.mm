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
  NSString * message =
      [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_cant_build_route"),
                                 L(@"dialog_routing_change_start_or_end")];
  return [self defaultAlertWithTitle:@"dialog_routing_unable_locate_route"
                             message:message
                    rightButtonTitle:@"ok"
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)locationServiceNotSupportedAlert
{
  kStatisticsEvent = @"Location Service Not Supported Alert";
  return [self defaultAlertWithTitle:@"device_doesnot_support_location_services"
                             message:nil
                    rightButtonTitle:@"ok"
                     leftButtonTitle:nil
                   rightButtonAction:nil];
}

+ (instancetype)noConnectionAlert
{
  kStatisticsEvent = @"No Connection Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"no_internet_connection_detected"
                                                message:nil
                                       rightButtonTitle:@"ok"
                                        leftButtonTitle:nil
                                      rightButtonAction:nil];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)unsavedEditsAlertWithOkBlock:(TMWMVoidBlock)okBlock
{
  kStatisticsEvent = @"Editor unsaved changes on delete";
  return [self defaultAlertWithTitle:@"editor_unsavde_changes"
                             message:nil
                    rightButtonTitle:@"delete"
                     leftButtonTitle:nil
                   rightButtonAction:okBlock];
}

+ (instancetype)noWiFiAlertWithName:(NSString *)name okBlock:(TMWMVoidBlock)okBlock
{
  kStatisticsEvent = @"No WiFi Alert";
  NSString * title = [NSString stringWithFormat:L(@"no_wifi_ask_cellular_download"), name];
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:title
                                                message:nil
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

+ (instancetype)internalErrorAlert
{
  kStatisticsEvent = @"Internal Error Alert";
  return [self defaultAlertWithTitle:@"dialog_routing_system_error"
                             message:nil
                    rightButtonTitle:@"ok"
                     leftButtonTitle:nil
                   rightButtonAction:nil];
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
      [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_error_location_not_found"),
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

+ (instancetype)downloaderNoConnectionAlertWithOkBlock:(TMWMVoidBlock)okBlock
{
  kStatisticsEvent = @"Downloader No Connection Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"downloader_error"
                                                message:@"downloader_no_internet_connection"
                                       rightButtonTitle:@"downloader_retry"
                                        leftButtonTitle:@"cancel"
                                      rightButtonAction:okBlock];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)downloaderNotEnoughSpaceAlert
{
  kStatisticsEvent = @"Downloader Not Enough Space Alert";
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"downloader_error"
                                                message:@"downloader_not_enough_space"
                                       rightButtonTitle:@"close"
                                        leftButtonTitle:nil
                                      rightButtonAction:nil];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)downloaderInternalErrorAlertForMap:(NSString *)name okBlock:(TMWMVoidBlock)okBlock
{
  kStatisticsEvent = @"Downloader Internal Error Alert";
  NSString * message =
      [NSString stringWithFormat:@"%@%@", L(@"downloader_internal_error_for_map"), name];
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"downloader_error"
                                                message:message
                                       rightButtonTitle:@"downloader_retry"
                                        leftButtonTitle:@"cancel"
                                      rightButtonAction:okBlock];
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

+ (instancetype)defaultAlertWithTitle:(nonnull NSString *)title
                              message:(nullable NSString *)message
                     rightButtonTitle:(nonnull NSString *)rightButtonTitle
                      leftButtonTitle:(nullable NSString *)leftButtonTitle
                    rightButtonAction:(nullable TMWMVoidBlock)action
{
  [[Statistics instance] logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatOpen}];
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
  [[Statistics instance] logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatApply}];
  if (self.rightButtonAction)
    self.rightButtonAction();
  [self close];
}

- (IBAction)leftButtonTap
{
  [[Statistics instance] logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatClose}];
  [self close];
}

@end
