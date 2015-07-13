//
//  MWMAlertViewController.m
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "Common.h"
#import "MWMAlertViewController.h"
#import "MWMDownloadTransitMapAlert.h"

static NSString * const kAlertControllerNibIdentifier = @"MWMAlertViewController";

@interface MWMAlertViewController () <UIGestureRecognizerDelegate>

@property (weak, nonatomic, readwrite) UIViewController * ownerViewController;

@end

@implementation MWMAlertViewController

- (instancetype)initWithViewController:(UIViewController *)viewController
{
  self = [super initWithNibName:kAlertControllerNibIdentifier bundle:nil];
  if (self)
    self.ownerViewController = viewController;
  return self;
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
  MWMAlert * alert = [self.view.subviews firstObject];
  if ([alert respondsToSelector:@selector(willRotateToInterfaceOrientation:)])
    [alert willRotateToInterfaceOrientation:toInterfaceOrientation];
}

#pragma mark - Actions

- (void)presentRateAlert
{
  [self displayAlert:MWMAlert.rateAlert];
}

- (void)presentLocationAlert
{
  [self displayAlert:MWMAlert.locationAlert];
}

- (void)presentFacebookAlert
{
  [self displayAlert:MWMAlert.facebookAlert];
}

- (void)presentLocationServiceNotSupportedAlert
{
  [self displayAlert:MWMAlert.locationServiceNotSupportedAlert];
}

- (void)presentNoConnectionAlert
{
  [self displayAlert:MWMAlert.noConnectionAlert];
}

- (void)presentnoWiFiAlertWithName:(NSString *)name downloadBlock:(RightButtonAction)block
{
  [self displayAlert:[MWMAlert noWiFiAlertWithName:name downloadBlock:block]];
}

- (void)presentFeedbackAlertWithStarsCount:(NSUInteger)starsCount
{
  [self displayAlert:[MWMAlert feedbackAlertWithStarsCount:starsCount]];
}

- (void)presentCrossCountryAlertWithCountries:(vector<storage::TIndex> const &)countries routes:(vector<storage::TIndex> const &)routes
{
  [self displayAlert:[MWMAlert crossCountryAlertWithCountries:countries routes:routes]];
}

- (void)presentDownloaderAlertWithCountries:(vector<storage::TIndex> const &)countries routes:(vector<storage::TIndex> const &)routes
{
  [self displayAlert:[MWMAlert downloaderAlertWithAbsentCountries:countries routes:routes]];
}

- (void)presentRoutingDisclaimerAlert
{
  [self displayAlert:MWMAlert.routingDisclaimerAlert];
}

- (void)presentDisabledLocationAlert
{
  [self displayAlert:MWMAlert.disabledLocationAlert];
}

- (void)presentAlert:(routing::IRouter::ResultCode)type
{
  [self displayAlert:[MWMAlert alert:type]];
}

- (void)displayAlert:(MWMAlert *)alert
{
  alert.alertController = self;
  [self.ownerViewController addChildViewController:self];
  self.view.center = self.ownerViewController.view.center;
  [self.ownerViewController.view addSubview:self.view];
  UIWindow * window = [[[UIApplication sharedApplication] delegate] window];
  [window addSubview:self.view];
  self.view.frame = window.bounds;
  [self.view addSubview:alert];
  alert.bounds = self.view.bounds;
  alert.center = self.view.center;
}

- (void)closeAlert
{
  self.ownerViewController.view.userInteractionEnabled = YES;
  [self.view removeFromSuperview];
  [self removeFromParentViewController];
}

@end
