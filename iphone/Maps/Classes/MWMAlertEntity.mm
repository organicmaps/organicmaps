//
//  MWMAlertEntity.m
//  Maps
//
//  Created by v.mikhaylenko on 06.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMAlertEntity.h"
#include <vector>
#include "../../../map/country_status_display.hpp"
#include <iostream>

@interface MWMAlertEntity ()

@end


@implementation MWMAlertEntity

+ (instancetype)entityWithType:(MWMAlertEntityType)type {
  MWMAlertEntity *entity = [[MWMAlertEntity alloc] init];
  switch (type) {
    case MWMAlertEntityTypeDownloader:
      [self setupEntityDownloaderAlert:entity];
      break;
  }
  return entity;
}

+ (void)setupEntityDownloaderAlert:(MWMAlertEntity *)entity {
  entity.title = @"Download all maps along your route";
  entity.message = @"Creating routes between regions is only possible when all the maps are downloaded and updates";
  entity.location = @"Chammpagne";
}



@end
