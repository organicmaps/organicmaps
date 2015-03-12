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
#import "MWMRouteNotFoundDefaultAlert.h"

@class MWMAlertEntity;

@implementation MWMAlert

+ (MWMAlert *)alertWithType:(MWMAlertType)type {
  switch (type) {
    case MWMAlertTypeDownloadTransitMap: {
      return [MWMDownloadTransitMapAlert alert];
    }
    case MWMAlertTypeDownloadAllMaps: {
      return [MWMDownloadAllMapsAlert alert];
    }
    case MWMAlertTypeRouteNotFoundDefault: {
      return [MWMRouteNotFoundDefaultAlert alert];
    }
  }
}

- (void)configureWithEntity:(MWMAlertEntity *)entity {
  [self doesNotRecognizeSelector:_cmd];
}

@end
