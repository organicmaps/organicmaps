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

@class MWMAlertViewController;

@interface MWMAlert : UIView

@property (weak, nonatomic) MWMAlertViewController *alertController;

+ (MWMAlert *)alert:(routing::IRouter::ResultCode)type;
+ (MWMAlert *)downloaderAlertWithAbsentCountries:(vector<storage::TIndex> const &)countries routes:(vector<storage::TIndex> const &)routes;
+ (MWMAlert *)crossCountryAlertWithCountries:(vector<storage::TIndex> const &)countries routes:(vector<storage::TIndex> const &)routes;
+ (MWMAlert *)rateAlert;
+ (MWMAlert *)feedbackAlertWithStarsCount:(NSUInteger)starsCount;
+ (MWMAlert *)facebookAlert;
+ (MWMAlert *)locationAlert;
+ (MWMAlert *)routingDisclaimerAlert;
+ (MWMAlert *)disabledLocationAlert;
+ (MWMAlert *)notWiFiAlertWithName:(NSString *)name downloadBlock:(void(^)())block;
+ (MWMAlert *)notConnectionAlert;
+ (MWMAlert *)locationServiceNotSupportedAlert;

- (void)setNeedsCloseAlertAfterEnterBackground;
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation;

@end
