//
//  MRMyTracker.h
//  MRMyTracker
//
//  Created by Igor Glotov on 22/07/14.
//  Copyright (c) 2014 Mailru Group. All rights reserved.
//  MyTracker, version 1.0.8

#import <Foundation/Foundation.h>
#import "MRTracker.h"

extern NSString * const MRMyTrackerVersion;

@class MRTracker;
@class MRTrackerParams;

@interface MRMyTracker : NSObject

+ (BOOL)enableLogging;

+ (void)setEnableLogging:(BOOL)enable;

+ (MRTracker *)initDefaultTrackerWithAppId:(NSString *)appId andParams:(MRTrackerParams *)params;

+ (MRTracker *)initDefaultTrackerWithAppId:(NSString *)appId;

+ (MRTracker *)getDefaultTracker;
@end
