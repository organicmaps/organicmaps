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
+ (MWMAlert *)downloaderAlertWithCountryIndex:(storage::TIndex const &)index;
+ (MWMAlert *)rateAlert;
+ (MWMAlert *)feedbackAlertWithStarsCount:(NSUInteger)starsCount;
+ (MWMAlert *)facebookAlert;

@end
