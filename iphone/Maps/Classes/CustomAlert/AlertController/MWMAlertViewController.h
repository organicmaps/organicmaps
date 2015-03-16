//
//  MWMAlertViewController.h
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MWMAlert.h"

#include "../../../../../routing/router.hpp"
#include "../../../../../std/vector.hpp"
#include "../../../../../storage/storage.hpp"

@interface MWMAlertViewController : UIViewController

@property (nonatomic, weak, readonly) UIViewController *ownerViewController;

- (instancetype)initWithViewController:(UIViewController *)viewController;
- (void)presentAlert:(routing::IRouter::ResultCode)type;
- (void)presentDownloaderAlertWithCountries:(vector<storage::TIndex> const&)countries;
- (void)closeAlert;

@end
