//
//  MWMRouteNotFoundDefaultAlert.m
//  Maps
//
//  Created by v.mikhaylenko on 12.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMDefaultAlert.h"
#import "MWMAlertViewController.h"
#import "UILabel+RuntimeAttributes.h"
#import "UIKitCategories.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMPlacePageViewManager.h"
#import "LocationManager.h"

#include "Framework.h"

typedef void (^RightButtonAction)();
static CGFloat const kDividerTopConstant = -8;

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

+ (instancetype)routeNotFoundAlert
{
  NSString * message = [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_cant_build_route"), L(@"dialog_routing_change_start_or_end")];
  return [self defaultAlertWithTitle:@"dialog_routing_unable_locate_route" message:message rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)locationServiceNotSupportedAlert
{
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"device_doesnot_support_location_services" message:nil rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
  return alert;
}

+ (instancetype)notConnectionAlert
{
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:@"no_internet_connection_detected" message:nil rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)notWiFiAlertWithName:(NSString *)name downloadBlock:(void(^)())block
{
  MWMDefaultAlert * alert = [self defaultAlertWithTitle:name message:nil rightButtonTitle:@"use_cellular_data" leftButtonTitle:@"cancel" rightButtonAction:block];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

+ (instancetype)endPointNotFoundAlert
{
  NSString * message = [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_end_not_determined"), L(@"dialog_routing_select_closer_end")];
  return [self defaultAlertWithTitle:@"dialog_routing_change_end" message:message rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)startPointNotFoundAlert
{
  NSString * message = [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_start_not_determined"), L(@"dialog_routing_select_closer_start")];
  return [self defaultAlertWithTitle:@"dialog_routing_change_start" message:message rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)internalErrorAlert
{
  NSString * message = [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_application_error"), L(@"dialog_routing_try_again")];
  return [self defaultAlertWithTitle:@"dialog_routing_system_errorr" message:message rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)noCurrentPositionAlert
{
  NSString * message = [NSString stringWithFormat:@"%@\n\n%@", L(@"dialog_routing_error_location_not_found"), L(@"dialog_routing_location_turn_wifi")];
  return [self defaultAlertWithTitle:@"dialog_routing_check_gps" message:message rightButtonTitle:@"OK" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)routingDisclaimerAlert
{
  NSString * message = [NSString stringWithFormat:@"%@\n\n%@\n\n%@\n\n%@", L(@"dialog_routing_disclaimer_priority"), L(@"dialog_routing_disclaimer_precision"), L(@"dialog_routing_disclaimer_recommendations"),L(@"dialog_routing_disclaimer_beware")];
  return [self defaultAlertWithTitle:@"dialog_routing_disclaimer_title" message:message rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)disabledLocationAlert
{
  RightButtonAction action = ^
  {
    MWMPlacePageViewManager * manager = [MapsAppDelegate theApp].m_mapViewController.placePageManager;
    [[MapsAppDelegate theApp].m_locationManager stop:(id<LocationObserver>)manager];
    GetFramework().GetLocationState()->SwitchToNextMode();
    [[MapsAppDelegate theApp].m_locationManager start:(id<LocationObserver>)manager];
  };
  return [MWMDefaultAlert defaultAlertWithTitle:@"dialog_routing_location_turn_on" message:@"dialog_routing_location_unknown_turn_on" rightButtonTitle:@"turn_on" leftButtonTitle:@"later" rightButtonAction:action];
}

+ (instancetype)pointsInDifferentMWMAlert
{
  return [self defaultAlertWithTitle:@"routing_failed_cross_mwm_building" message:nil rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)defaultAlertWithTitle:(nonnull NSString *)title message:(nullable NSString *)message rightButtonTitle:(nonnull NSString *)rightButtonTitle leftButtonTitle:(nullable NSString *)leftButtonTitle rightButtonAction:(nullable RightButtonAction)action
{
  MWMDefaultAlert * alert = [[[NSBundle mainBundle] loadNibNamed:kDefaultAlertNibName owner:self options:nil] firstObject];
  alert.titleLabel.localizedText = title;
  alert.messageLabel.localizedText = message;
  if (!message)
  {
    alert.dividerTop.constant = kDividerTopConstant;
    [alert layoutSubviews];
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
    alert.rightButtonWidth.constant = alert.width;
  }
  return alert;
}

#pragma mark - Actions

- (IBAction)rightButtonTap
{
  if (self.rightButtonAction)
    self.rightButtonAction();
  [self.alertController closeAlert];
}

- (IBAction)leftButtonTap
{
  [self.alertController closeAlert];
}

@end
