//
//  MWMAlert.m
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMAlert.h"
#import "MWMDownloadTransitMapAlert.h"
#import "MWMDownloadAllMapsAlert.h"
#import "MWMDefaultAlert.h"
#import "MWMFeedbackAlert.h"
#import "MWMRateAlert.h"
#import "MWMFacebookAlert.h"

extern UIColor * const kActiveDownloaderViewColor = [UIColor colorWithRed:211/255. green:209/255. blue:205/255. alpha:1.];

@implementation MWMAlert

+ (MWMAlert *)rateAlert
{
  return [MWMRateAlert alert];
}

+ (MWMAlert *)facebookAlert
{
  return [MWMFacebookAlert alert];
}

+ (MWMAlert *)feedbackAlertWithStarsCount:(NSUInteger)starsCount
{
  return [MWMFeedbackAlert alertWithStarsCount:starsCount];
}

+ (MWMAlert *)downloaderAlertWithCountryIndex:(const storage::TIndex&)index
{
  return [MWMDownloadTransitMapAlert alertWithCountryIndex:index];
}

+ (MWMAlert *)alert:(routing::IRouter::ResultCode)type
{
  switch (type)
  {
      
    case routing::IRouter::NoCurrentPosition:
      return [MWMDefaultAlert noCurrentPositionAlert];
          
    case routing::IRouter::InconsistentMWMandRoute:
    case routing::IRouter::RouteFileNotExist:
      return [MWMDownloadAllMapsAlert alert];
      
    case routing::IRouter::StartPointNotFound:
      return [MWMDefaultAlert startPointNotFoundAlert];
      
    case routing::IRouter::EndPointNotFound:
      return [MWMDefaultAlert endPointNotFoundAlert];
      
    case routing::IRouter::PointsInDifferentMWM:
      return [MWMDefaultAlert pointsInDifferentMWMAlert];
      
    case routing::IRouter::RouteNotFound:
      return [MWMDefaultAlert routeNotFoundAlert];
      
    case routing::IRouter::InternalError:
      return [MWMDefaultAlert internalErrorAlert];
      
    case routing::IRouter::Cancelled:
    case routing::IRouter::NoError:
    case routing::IRouter::NeedMoreMaps:
      return nil;
  }
}

@end
