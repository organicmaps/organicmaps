//
//  MWMAlertEntity.m
//  Maps
//
//  Created by v.mikhaylenko on 06.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMAlertEntity.h"
#include <string>

@interface MWMAlertEntity ()

@end


@implementation MWMAlertEntity

- (instancetype)initWithCallbackString:(const std::string &)string {
  self = [super init];
  if (self) {
    [self processCallbackString:string];
  }
  return self;
}

- (void)processCallbackString:(const std::string &)string {
  
}

@end
