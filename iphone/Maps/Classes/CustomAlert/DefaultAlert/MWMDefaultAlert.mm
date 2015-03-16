//
//  MWMRouteNotFoundDefaultAlert.m
//  Maps
//
//  Created by v.mikhaylenko on 12.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMDefaultAlert.h"
#import "MWMAlertViewController.h"
#import "MWMDefaultAlert+Configure.h"
#import "UILabel+RuntimeAttributes.h"

@interface MWMDefaultAlert ()

@property (nonatomic, weak, readwrite) IBOutlet UILabel *messageLabel;
@property (nonatomic, weak, readwrite) IBOutlet UIButton *okButton;
@property (nonatomic, weak, readwrite) IBOutlet UIView *deviderLine;

@end

static NSString * const kDefaultAlertNibName = @"MWMDefaultAlert";

@implementation MWMDefaultAlert

+ (instancetype)routeNotFoundAlert {
  return [self defaultAlertWithLocalizedText:@"routing_failed_route_not_found"];
}

+ (instancetype)endPointNotFoundAlert {
  return [self defaultAlertWithLocalizedText:@"routing_failed_dst_point_not_found"];
}

+ (instancetype)startPointNotFoundAlert {
  return [self defaultAlertWithLocalizedText:@"routing_failed_start_point_not_found"];
}

+ (instancetype)internalErrorAlert {
  return [self defaultAlertWithLocalizedText:@"routing_failed_internal_error"];
}

+ (instancetype)noCurrentPositionAlert {
  return [self defaultAlertWithLocalizedText:@"routing_failed_unknown_my_position"];
}

+ (instancetype)pointsInDifferentMWMAlert {
  return [self defaultAlertWithLocalizedText:@"routing_failed_cross_mwm_building"];
}

+ (instancetype)defaultAlertWithLocalizedText:(NSString *)text {
  MWMDefaultAlert *alert = [[[NSBundle mainBundle] loadNibNamed:kDefaultAlertNibName owner:self options:nil] firstObject];
  alert.messageLabel.localizedText = text;
  [alert configure];
  return alert;
}

#pragma mark - Actions

- (IBAction)okButtonTap:(id)sender {
  [self.alertController closeAlert];
}

@end
