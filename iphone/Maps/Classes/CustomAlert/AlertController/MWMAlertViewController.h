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
- (void)presentDownloaderAlertWithCountries:(vector<storage::TIndex> const &)countries routes:(vector<storage::TIndex> const &)routes;
- (void)presentCrossCountryAlertWithCountries:(vector<storage::TIndex> const &)countries routes:(vector<storage::TIndex> const &)routes;
- (void)presentRateAlert;
- (void)presentFacebookAlert;
- (void)presentFeedbackAlertWithStarsCount:(NSUInteger)starsCount;
- (void)presentRoutingDisclaimerAlert;
- (void)presentDisabledLocationAlert;
- (void)presentLocationAlert;
- (void)presentLocationServiceNotSupportedAlert;
- (void)presentNotConnectionAlert;
- (void)presentNotWifiAlertWithName:(NSString *)name downloadBlock:(void(^)())block;

- (void)closeAlert;

- (instancetype)init __attribute__((unavailable("-init isn't available, call -initWithViewController: instead!")));
+ (instancetype)new __attribute__((unavailable("+new isn't available, call -initWithViewController: instead!")));
- (instancetype)initWithCoder:(NSCoder *)aDecoder __attribute__((unavailable("-initWithCoder: isn't available, call -initWithViewController: instead!")));
- (instancetype)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil __attribute__((unavailable("-initWithNibName:bundle: isn't available, call -initWithViewController: instead!")));
@end
