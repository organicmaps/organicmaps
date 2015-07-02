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

@interface MWMDefaultAlert ()

@property (weak, nonatomic) IBOutlet UILabel * messageLabel;
@property (weak, nonatomic) IBOutlet UIButton * rightButton;
@property (weak, nonatomic) IBOutlet UIButton * leftButton;
@property (weak, nonatomic) IBOutlet UILabel * titleLabel;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * rightButtonWidth;

@end

static NSString * const kDefaultAlertNibName = @"MWMDefaultAlert";

@implementation MWMDefaultAlert

+ (instancetype)routeNotFoundAlert
{
  return [self defaultAlertWithLocalizedText:@"routing_failed_route_not_found"];
}

+ (instancetype)endPointNotFoundAlert
{
  return [self defaultAlertWithLocalizedText:@"routing_failed_dst_point_not_found"];
}

+ (instancetype)startPointNotFoundAlert
{
  return [self defaultAlertWithLocalizedText:@"routing_failed_start_point_not_found"];
}

+ (instancetype)internalErrorAlert
{
  return [self defaultAlertWithLocalizedText:@"routing_failed_internal_error"];
}

+ (instancetype)noCurrentPositionAlert
{
  return [self defaultAlertWithLocalizedText:@"routing_failed_unknown_my_position"];
}

+ (instancetype)pointsInDifferentMWMAlert
{
  return [self defaultAlertWithLocalizedText:@"routing_failed_cross_mwm_building"];
}

+ (instancetype)defaultAlertWithLocalizedText:(NSString *)text
{
  MWMDefaultAlert * alert = [[[NSBundle mainBundle] loadNibNamed:kDefaultAlertNibName owner:self options:nil] firstObject];
  alert.messageLabel.localizedText = text;
  return alert;
}

+ (instancetype)defaultAlertWithTitle:(nonnull NSString *)title message:(nonnull NSString *)message rightButtonTitle:(nonnull NSString *)rightButtonTitle leftButtonTitle:(nullable NSString *)leftButtonTitle
{
  MWMDefaultAlert * alert = [[[NSBundle mainBundle] loadNibNamed:kDefaultAlertNibName owner:self options:nil] firstObject];
  alert.messageLabel.localizedText = message;
  alert.titleLabel.localizedText = title;
  alert.rightButton.localizedText = rightButtonTitle;
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
  [self.alertController closeAlert];
}

- (IBAction)leftButtonTap
{
  [self.alertController closeAlert];
}

@end
