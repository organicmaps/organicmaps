//
//  MWMAlertViewController.h
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MWMAlert.h"

#include "routing/router.hpp"
#include "storage/storage.hpp"

@interface MWMAlertViewController : UIViewController

@property (weak, nonatomic, readonly) UIViewController * ownerViewController;

- (instancetype)initWithViewController:(UIViewController *)viewController;
- (void)presentAlert:(routing::IRouter::ResultCode)type;
- (void)presentDownloaderAlertWithCountryIndex:(storage::TIndex const &)index;
- (void)presentRateAlert;
- (void)presentFacebookAlert;
- (void)presentFeedbackAlertWithStarsCount:(NSUInteger)starsCount;
- (void)closeAlert;

@end
