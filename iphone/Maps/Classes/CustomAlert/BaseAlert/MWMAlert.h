//
//  MWMAlert.h
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

#include "../../../../../routing/router.hpp"
#include "../../../../../std/vector.hpp"
#include "../../../../../storage/storage.hpp"

@class MWMAlertViewController;

@interface MWMAlert : UIView
@property (nonatomic, weak) MWMAlertViewController *alertController;

+ (MWMAlert *)alert:(routing::IRouter::ResultCode)type;
+ (MWMAlert *)downloaderAlertWithCountries:(vector<storage::TIndex> const&)countries;

@end
