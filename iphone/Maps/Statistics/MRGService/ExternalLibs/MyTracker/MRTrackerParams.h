//
// Created by Igor Glotov on 22/07/14.
// Copyright (c) 2014 Mailru Group. All rights reserved.
// MyTracker, version 1.0.9

#import <Foundation/Foundation.h>

@class MRCustomParamsProvider;


@interface MRTrackerParams : NSObject

@property (nonatomic) BOOL trackAppLaunch;
@property (strong, nonatomic) MRCustomParamsProvider * customParamsProvider;

@end