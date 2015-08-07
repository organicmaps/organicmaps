//
// Created by Glotov on 24/03/15.
// Copyright (c) 2015 Mail.ru Group. All rights reserved.
// MyTracker, version 1.1.1

#import <Foundation/Foundation.h>
#import "MRTrackerParams.h"

#import "MRCustomParamsProvider.h"

@interface MRTrackerParams (Corp)

- (MRCustomParamsProvider *)getCustomParams;

- (NSTimeInterval)launchTimeout;
- (void)setLaunchTimeout:(NSTimeInterval)launchTimeout;


@end
