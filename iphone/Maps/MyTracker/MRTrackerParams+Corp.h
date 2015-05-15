//
// Created by Glotov on 24/03/15.
// Copyright (c) 2015 Mail.ru Group. All rights reserved.
// MyTracker, version 1.0.18

#import <Foundation/Foundation.h>
#import "MRTrackerParams.h"

@class MRCustomParamsProvider;

@interface MRTrackerParams (Corp)

- (MRCustomParamsProvider *)getCustomParams;

- (NSTimeInterval)launchTimeout;
- (void)setLaunchTimeout:(NSTimeInterval)launchTimeout;


@end
