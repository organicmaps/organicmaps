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

@interface MWMDefaultAlert ()

@property (weak, nonatomic) IBOutlet UILabel * messageLabel;
@property (weak, nonatomic) IBOutlet UIButton * rightButton;
@property (weak, nonatomic) IBOutlet UIButton * leftButton;
@property (weak, nonatomic) IBOutlet UILabel * titleLabel;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * rightButtonWidth;
@property (copy, nonatomic) RightButtonAction rightButtonAction;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint *dividerTop;

@end

static NSString * const kDefaultAlertNibName = @"MWMDefaultAlert";

@implementation MWMDefaultAlert

+ (instancetype)routeFileNotExistAlert
{
  [Statistics.instance logEvent:@"Route File Not Exist Alert - open"];
  return [self defaultAlertWithTitle:@"dialog_routing_download_files" message:@"dialog_routing_download_and_update_all" rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)routeNotFoundAlert
{
  [Statistics.instance logEvent:@"Route Not Found Alert - open"];
  NSString * message = [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_cant_build_route"), L(@"dialog_routing_change_start_or_end")];
  return [self defaultAlertWithTitle:@"dialog_routing_unable_locate_route" message:message rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)locationServiceNotSupportedAlert
{
  [Statistics.instance logEvent:@"Location Service Not Supported Alert - open"];
  return [self defaultAlertWithTitle:@"device_doesnot_support_location_services" message:nil rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)noConnectionAlert
{
  [Statistics.instance logEvent:@"No Connection Alert - open"];
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"no_internet_connection_detected" message:nil rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)noWiFiAlertWithName:(NSString *)name downloadBlock:(RightButtonAction)block
{
  [Statistics.instance logEvent:@"No WiFi Alert - open" withParameters:@{@"map_name" : name}];
  NSString * title = [NSString stringWithFormat:L(@"no_wifi_ask_cellular_download"), name];
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:title message:nil rightButtonTitle:@"use_cellular_data" leftButtonTitle:@"cancel" rightButtonAction:block];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)endPointNotFoundAlert
{
  [Statistics.instance logEvent:@"End Point Not Found Alert - open"];
  NSString * message = [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_end_not_determined"), L(@"dialog_routing_select_closer_end")];
  return [self defaultAlertWithTitle:@"dialog_routing_change_end" message:message rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)startPointNotFoundAlert
{
  [Statistics.instance logEvent:@"Start Point Not Found Alert - open"];
  NSString * message = [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_start_not_determined"), L(@"dialog_routing_select_closer_start")];
  return [self defaultAlertWithTitle:@"dialog_routing_change_start" message:message rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)internalErrorAlert
{
  [Statistics.instance logEvent:@"Internal Error Alert - open"];
  NSString * message = [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_application_error"), L(@"dialog_routing_try_again")];
  return [self defaultAlertWithTitle:@"dialog_routing_system_error" message:message rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)noCurrentPositionAlert
{
  [Statistics.instance logEvent:@"No Current Position Alert - open"];
  NSString * message = [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_error_location_not_found"), L(@"dialog_routing_location_turn_wifi")];
  return [self defaultAlertWithTitle:@"dialog_routing_check_gps" message:message rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)disabledLocationAlert
{
  [Statistics.instance logEvent:@"Disabled Location Alert - open"];
  RightButtonAction action = ^
  {
    GetFramework().GetLocationState()->SwitchToNextMode();
  };
  return [MWMDefaultAlert defaultAlertWithTitle:@"dialog_routing_location_turn_on" message:@"dialog_routing_location_unknown_turn_on" rightButtonTitle:@"turn_on" leftButtonTitle:@"later" rightButtonAction:action];
}

+ (instancetype)pointsInDifferentMWMAlert
{
  [Statistics.instance logEvent:@"Points In Different MWM Alert - open"];
  return [self defaultAlertWithTitle:@"routing_failed_cross_mwm_building" message:nil rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)point2PointAlertWithOkBlock:(RightButtonAction)block
{
  return [self defaultAlertWithTitle:@"Navigation is available only from your current location."
                             message:@"Do you want us to recreate/rebuild/ plan the route?"
                    rightButtonTitle:@"ok" leftButtonTitle:@"cancel" rightButtonAction:block];
}

+ (instancetype)defaultAlertWithTitle:(nonnull NSString *)title message:(nullable NSString *)message rightButtonTitle:(nonnull NSString *)rightButtonTitle leftButtonTitle:(nullable NSString *)leftButtonTitle rightButtonAction:(nullable RightButtonAction)action
{
  MWMDefaultAlert * alert = [[[NSBundle mainBundle] loadNibNamed:kDefaultAlertNibName owner:self options:nil] firstObject];
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
  [Statistics.instance logEvent:@"Default Alert - rightButtonTap"];
  if (self.rightButtonAction)
    self.rightButtonAction();
  [self close];
}

- (IBAction)leftButtonTap
{
  [Statistics.instance logEvent:@"Default Alert - leftButtonTap"];
  [self close];
}

@end
