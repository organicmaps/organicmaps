//
//  MWMWatchEventInfo.m
//  Maps
//
//  Created by i.grechuhin on 10.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMWatchEventInfo.h"

static NSString * const kRequestKey = @"request";

@implementation NSDictionary (MWMWatchEventInfo)

- (MWMWatchEventInfoRequest)watchEventInfoRequest
{
  NSNumber * requestValue = self[kRequestKey];
  return (MWMWatchEventInfoRequest)requestValue.unsignedIntegerValue;
}

@end

@implementation NSMutableDictionary (MWMWatchEventInfo)

- (void)setWatchEventInfoRequest:(MWMWatchEventInfoRequest)watchEventInfoRequest
{
  NSNumber * requestValue = @(watchEventInfoRequest);
  self[kRequestKey] = requestValue;
}

@end
