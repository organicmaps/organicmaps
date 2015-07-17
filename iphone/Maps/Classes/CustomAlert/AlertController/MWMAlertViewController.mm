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
#import "UIKitCategories.h"

static NSString * const kAlertControllerNibIdentifier = @"MWMAlertViewController";

@interface MWMAlertViewController () <UIGestureRecognizerDelegate, UIAlertViewDelegate>

@property (nonnull ,weak, nonatomic, readwrite) UIViewController * ownerViewController;

@end

@implementation MWMAlertViewController

- (nonnull instancetype)initWithViewController:(nonnull UIViewController *)viewController
{
  self = [super initWithNibName:kAlertControllerNibIdentifier bundle:nil];
  if (self)
    self.ownerViewController = viewController;
  return self;
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
  MWMAlert * alert = [self.view.subviews firstObject];
  if (isIOSVersionLessThan(8) && [alert respondsToSelector:@selector(setTransform:)])
  {
    [UIView animateWithDuration:duration animations:^
    {
      alert.transform = [self rotationForOrientation:toInterfaceOrientation];
    }];
  }
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
  NSString * title = L(@"location_is_disabled_long_text");
  NSString * cancel = L(@"cancel");
  NSString * openSettings = L(@"settings");
  if (isIOSVersionLessThan(8))
  {
    UIAlertView * alertView = [[UIAlertView alloc] initWithTitle:title message:nil delegate:self cancelButtonTitle:cancel otherButtonTitles:openSettings, nil];
    [alertView show];
    return;
  }
  UIAlertController * alertController = [UIAlertController alertControllerWithTitle:title message:nil preferredStyle:UIAlertControllerStyleAlert];
  UIAlertAction * cancelAction = [UIAlertAction actionWithTitle:cancel style:UIAlertActionStyleCancel handler:nil];
  UIAlertAction * openSettingsAction = [UIAlertAction actionWithTitle:openSettings style:UIAlertActionStyleDefault handler:^(UIAlertAction * action)
  {
    [self openSettings];
  }];
  [alertController addAction:cancelAction];
  [alertController addAction:openSettingsAction];
  [self.ownerViewController presentViewController:alertController animated:YES completion:nil];
//  dispatch_async(dispatch_get_main_queue(), ^
//  {
//    // @TODO Remove dispatch on LocationManager -> MWMLocationManager
//    // Test case when location is denied by user on app launch/relaunch
//    [self displayAlert:MWMAlert.locationAlert];
//  });
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

- (void)presentnoWiFiAlertWithName:(nonnull NSString *)name downloadBlock:(nullable RightButtonAction)block
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
  [self displayAlert:[MWMAlert routingDisclaimerAlertWithInitialOrientation:self.ownerViewController.interfaceOrientation]];
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
  self.view.alpha = 0.;
  alert.alpha = 0.;
  CGFloat const scale = 1.1;
  alert.transform = CGAffineTransformMakeScale(scale, scale);
  self.view.center = self.ownerViewController.view.center;
  [self.ownerViewController.view addSubview:self.view];
  UIWindow * window = [[[UIApplication sharedApplication] delegate] window];
  [window addSubview:self.view];
  self.view.frame = window.bounds;
  if (isIOSVersionLessThan(8))
    alert.transform = [self rotationForOrientation:self.ownerViewController.interfaceOrientation];
  [self.view addSubview:alert];
  alert.bounds = self.view.bounds;
  alert.center = self.view.center;
  [UIView animateWithDuration:.15 animations:^
  {
    self.view.alpha = 1.;
    alert.alpha = 1.;
    alert.transform = CGAffineTransformIdentity;
  }];
}

- (void)closeAlertWithCompletion:(nullable CloseAlertCompletion)completion
{
  MWMAlert * alert = self.view.subviews.firstObject;
  [UIView animateWithDuration:.15 animations:^
  {
    alert.alpha = 0.;
    self.view.alpha = 0.;
  }
  completion:^(BOOL finished)
  {
    if (completion)
      completion();
    [self.view removeFromSuperview];
    [self removeFromParentViewController];
  }];
}

- (void)openSettings
{
  NSURL * url = [NSURL URLWithString:UIApplicationOpenSettingsURLString];
  UIApplication * a = [UIApplication sharedApplication];
  if ([a canOpenURL:url])
    [a openURL:url];
}

- (CGAffineTransform)rotationForOrientation:(UIInterfaceOrientation)orientation
{
  switch (orientation)
  {
    case UIInterfaceOrientationLandscapeLeft:
      return CGAffineTransformMakeRotation(-M_PI_2);
    case UIInterfaceOrientationLandscapeRight:
      return CGAffineTransformMakeRotation(M_PI_2);
    case UIInterfaceOrientationPortraitUpsideDown:
      return CGAffineTransformMakeRotation(M_PI);
    case UIInterfaceOrientationUnknown:
    case UIInterfaceOrientationPortrait:
      return CGAffineTransformIdentity;
  }
}

#pragma mark - UIAlertViewDelegate

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
  if (buttonIndex == 1)
    [self openSettings];
}


@end
