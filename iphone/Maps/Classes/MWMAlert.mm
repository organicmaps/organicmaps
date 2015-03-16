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

extern UIColor * const kActiveDownloaderViewColor = [UIColor colorWithRed:211/255. green:209/255. blue:205/255. alpha:1.];

@implementation MWMAlert

+ (MWMAlert *)downloaderAlertWithCountries:(const vector<storage::TIndex> &)countries {
  return [MWMDownloadTransitMapAlert alertWithCountries:countries];
}

+ (MWMAlert *)alert:(routing::IRouter::ResultCode)type {
  switch (type) {
      
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
      return nil;
  }
}

@end
