//
//  MWMAlert.h
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

#include "routing/router.hpp"
#include "storage/storage.hpp"

typedef void (^RightButtonAction)();

@class MWMAlertViewController;
@interface MWMAlert : UIView

@property (weak, nonatomic) MWMAlertViewController * alertController;

+ (MWMAlert *)alert:(routing::IRouter::ResultCode)type;
+ (MWMAlert *)downloaderAlertWithAbsentCountries:(vector<storage::TIndex> const &)countries
                                          routes:(vector<storage::TIndex> const &)routes
                                            code:(routing::IRouter::ResultCode)code;
+ (MWMAlert *)rateAlert;
+ (MWMAlert *)feedbackAlertWithStarsCount:(NSUInteger)starsCount;
+ (MWMAlert *)facebookAlert;
+ (MWMAlert *)locationAlert;
+ (MWMAlert *)routingDisclaimerAlertWithInitialOrientation:(UIInterfaceOrientation)orientation;
+ (MWMAlert *)disabledLocationAlert;
+ (MWMAlert *)noWiFiAlertWithName:(NSString *)name downloadBlock:(RightButtonAction)block;
+ (MWMAlert *)noConnectionAlert;
+ (MWMAlert *)locationServiceNotSupportedAlert;
- (void)close;

- (void)setNeedsCloseAlertAfterEnterBackground;
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation;

@end
