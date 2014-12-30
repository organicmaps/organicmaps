//
// Created by Igor Glotov on 05/08/14.
// Copyright (c) 2014 Mailru Group. All rights reserved.
// MyTracker, version 1.0.9

#import <Foundation/Foundation.h>
#import "MRMyTracker.h"


@interface MCMyTracker : NSObject

+ (BOOL)enableLogging;

+ (void)setEnableLogging:(BOOL)enable;

+ (MRTracker *)initDefaultTrackerWithAppId:(NSString *)appId andParams:(MRTrackerParams *)params;

+ (MRTracker *)initDefaultTrackerWithAppId:(NSString *)appId;

+ (MRTracker *)getDefaultTracker;

@end