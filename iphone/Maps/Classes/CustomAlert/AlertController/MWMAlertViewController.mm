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
  MWMAlert * alert = [MWMAlert rateAlert];
  [self displayAlert:alert];
}

- (void)presentLocationAlert
{
  MWMAlert * alert = [MWMAlert locationAlert];
  [self displayAlert:alert];
}

- (void)presentFacebookAlert
{
  MWMAlert * alert = [MWMAlert facebookAlert];
  [self displayAlert:alert];
}

- (void)presentLocationServiceNotSupportedAlert
{
  MWMAlert * alert = [MWMAlert locationServiceNotSupportedAlert];
  [self displayAlert:alert];
}

- (void)presentNotConnectionAlert
{
  MWMAlert * alert = [MWMAlert notConnectionAlert];
  [self displayAlert:alert];
}

- (void)presentNotWifiAlertWithName:(NSString *)name downloadBlock:(void(^)())block
{
  MWMAlert * alert = [MWMAlert notWiFiAlertWithName:name downloadBlock:block];
  [self displayAlert:alert];
}

- (void)presentFeedbackAlertWithStarsCount:(NSUInteger)starsCount
{
  MWMAlert * alert = [MWMAlert feedbackAlertWithStarsCount:starsCount];
  [self displayAlert:alert];
}

- (void)presentDownloaderAlertWithCountries:(vector<storage::TIndex> const &)countries routes:(vector<storage::TIndex> const &)routes
{
  MWMAlert * alert = [MWMAlert downloaderAlertWithAbsentCountries:countries routes:routes];
  [self displayAlert:alert];
}

- (void)presentRoutingDisclaimerAlert
{
  MWMAlert * alert = [MWMAlert routingDisclaimerAlert];
  [self displayAlert:alert];
}

- (void)presentDisabledLocationAlert
{
  MWMAlert * alert = [MWMAlert disabledLocationAlert];
  [self displayAlert:alert];
}

- (void)presentAlert:(routing::IRouter::ResultCode)type
{
  MWMAlert * alert = [MWMAlert alert:type];
  [self displayAlert:alert];
}

- (void)displayAlert:(MWMAlert *)alert
{
  alert.alertController = self;
  [self.ownerViewController addChildViewController:self];
  self.view.center = self.ownerViewController.view.center;
  [self.ownerViewController.view addSubview:self.view];
  [[[[UIApplication sharedApplication] delegate] window] addSubview:self.view];
  self.view.frame = [[[[UIApplication sharedApplication] delegate] window] frame];
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
