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
  return [self defaultAlertWithTitle:@"route_not_found" message:@"routing_failed_route_not_found" rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)endPointNotFoundAlert
{
  return [self defaultAlertWithTitle:@"change_final_point" message:@"routing_failed_dst_point_not_found" rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)startPointNotFoundAlert
{
  return [self defaultAlertWithTitle:@"change_start_point" message:@"routing_failed_start_point_not_found" rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)routeNotExist
{
  return [self defaultAlertWithTitle:@"ggg" message:@"route_not_exist" rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)internalErrorAlert
{
  return [self defaultAlertWithTitle:@"internal_error" message:@"routing_failed_internal_error" rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)noCurrentPositionAlert
{
  return [self defaultAlertWithTitle:@"check_gps" message:@"routing_failed_unknown_my_position" rightButtonTitle:@"OK" leftButtonTitle:nil rightButtonAction:nil];
}

+ (instancetype)routingDisclaimerAlert
{
  return [self defaultAlertWithTitle:@"gggg" message:@"routing_disclaimer" rightButtonTitle:@"ok" leftButtonTitle:nil rightButtonAction:nil];
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
  return [MWMDefaultAlert defaultAlertWithTitle:L(@"turn_on_geolocation") message:L(@"turn_geolaction_on") rightButtonTitle:L(@"turn_on") leftButtonTitle:L(@"not_now") rightButtonAction:action];
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
