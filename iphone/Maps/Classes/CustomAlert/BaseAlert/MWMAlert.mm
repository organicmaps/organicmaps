//
//  MWMAlert.m
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMAlert.h"
#import "MWMAlertViewController.h"
#import "MWMDefaultAlert.h"
#import "MWMDownloadTransitMapAlert.h"
#import "MWMFacebookAlert.h"
#import "MWMFeedbackAlert.h"
#import "MWMLocationAlert.h"
#import "MWMRateAlert.h"

@implementation MWMAlert

+ (MWMAlert *)rateAlert
{
  return [MWMRateAlert alert];
}

+ (MWMAlert *)locationAlert
{
  return [MWMLocationAlert alert];
}

+ (MWMAlert *)facebookAlert
{
  return [MWMFacebookAlert alert];
}

+ (MWMAlert *)routingDisclaimerAlert
{
  return [MWMDefaultAlert routingDisclaimerAlert];
}

+ (MWMAlert *)disabledLocationAlert
{
  return [MWMDefaultAlert disabledLocationAlert];
}

+ (MWMAlert *)noWiFiAlertWithName:(NSString *)name downloadBlock:(RightButtonAction)block
{
  return [MWMDefaultAlert noWiFiAlertWithName:name downloadBlock:block];
}

+ (MWMAlert *)noConnectionAlert
{
  return [MWMDefaultAlert noConnectionAlert];
}

+ (MWMAlert *)locationServiceNotSupportedAlert
{
  return [MWMDefaultAlert locationServiceNotSupportedAlert];
}

+ (MWMAlert *)feedbackAlertWithStarsCount:(NSUInteger)starsCount
{
  return [MWMFeedbackAlert alertWithStarsCount:starsCount];
}

+ (MWMAlert *)crossCountryAlertWithCountries:(vector<storage::TIndex> const &)countries routes:(vector<storage::TIndex> const &)routes
{
  return [MWMDownloadTransitMapAlert crossCountryAlertWithMaps:countries routes:routes];
}

+ (MWMAlert *)downloaderAlertWithAbsentCountries:(vector<storage::TIndex> const &)countries routes:(vector<storage::TIndex> const &)routes
{
  return [MWMDownloadTransitMapAlert downloaderAlertWithMaps:countries routes:routes];
}

+ (MWMAlert *)alert:(routing::IRouter::ResultCode)type
{
  switch (type)
  {
    case routing::IRouter::NoCurrentPosition:
      return [MWMDefaultAlert noCurrentPositionAlert];
    case routing::IRouter::StartPointNotFound:
      return [MWMDefaultAlert startPointNotFoundAlert];
    case routing::IRouter::EndPointNotFound:
      return [MWMDefaultAlert endPointNotFoundAlert];
    case routing::IRouter::PointsInDifferentMWM:
      return [MWMDefaultAlert pointsInDifferentMWMAlert];
    case routing::IRouter::RouteNotFound:
    case routing::IRouter::InconsistentMWMandRoute:
      return [MWMDefaultAlert routeNotFoundAlert];
    case routing::IRouter::RouteFileNotExist:
      return [MWMDefaultAlert routeFileNotExistAlert];
    case routing::IRouter::InternalError:
      return [MWMDefaultAlert internalErrorAlert];
    case routing::IRouter::Cancelled:
    case routing::IRouter::NoError:
    case routing::IRouter::NeedMoreMaps:
      return nil;
  }
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
// Should override this method if you want custom relayout after rotation.
}

- (void)close
{
  [self.alertController closeAlertWithCompletion:^
  {
    [self removeFromSuperview];
  }];
}

- (void)setNeedsCloseAlertAfterEnterBackground
{
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationDidEnterBackground) name:UIApplicationDidEnterBackgroundNotification object:nil];
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)applicationDidEnterBackground
{
// Should close alert when application entered background.
  [self close];
}

@end
