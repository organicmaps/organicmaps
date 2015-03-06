//
//  MWMAlert.m
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMAlert.h"
#import "MWMDownloadAllMapsAlert.h"

@class MWMAlertEntity;

@implementation MWMAlert

+ (MWMAlert *)alertWithType:(MWMAlertType)type {
  switch (type) {
    case MWMAlertTypeDownloadAllMaps: {
      return [MWMDownloadAllMapsAlert alert];
    }
  }
}

- (void)configureWithEntity:(MWMAlertEntity *)entity {
  [self doesNotRecognizeSelector:_cmd];
}

@end
