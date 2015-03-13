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

@class MWMAlertEntity;

@implementation MWMAlert

+ (MWMAlert *)alertWithType:(MWMAlertType)type {
  switch (type) {
    case MWMAlertTypeDownloadTransitMap:
      return [MWMDownloadTransitMapAlert alert];
    
    case MWMAlertTypeDownloadAllMaps:
      return [MWMDownloadAllMapsAlert alert];
    
    case MWMAlertTypeRouteNotFound:
      return [MWMDefaultAlert routeNotFoundAlert];
    
    case MWMAlertTypeStartPointNotFound:
      return [MWMDefaultAlert startPointNotFoundAlert];
      
    case MWMAlertTypeEndPointNotFound:
      return [MWMDefaultAlert endPointNotFoundAlert];
      
    case MWMAlertTypeInconsistentMWMandRoute:
      return [MWMDownloadAllMapsAlert alert];
      
    case MWMAlertTypeInternalError:
      return [MWMDefaultAlert internalErrorAlert];
      
    case MWMAlertTypeNoCurrentPosition:
      return [MWMDefaultAlert noCurrentPositionAlert];
      
    case MWMAlertTypePointsInDifferentMWM:
      return [MWMDefaultAlert pointsInDifferentMWMAlert];
      
  }
}

- (void)configureWithEntity:(MWMAlertEntity *)entity {
  [self doesNotRecognizeSelector:_cmd];
}

@end
