// Created by Igor Glotov on 22/07/14.
// Copyright (c) 2014 Mailru Group. All rights reserved.
// MyTracker, version 1.1.1

#import <Foundation/Foundation.h>
#import "MRTracker.h"

extern NSString * const MRMyTrackerVersion;

@class MRTracker;
@class MRTrackerParams;

@interface MRMyTracker : NSObject

+ (BOOL)debugMode;
+ (void)setDebugMode:(BOOL)enable;

+ (MRTracker *)createTracker:(NSString *)trackerId;
+ (BOOL)isInitialized;
+ (void)setupTracker;

+ (MRTrackerParams *)getTrackerParams;

+ (void)trackLoginEvent;
+ (void)trackLoginEventWithParams:(NSDictionary *)eventParams;

+ (void)trackInviteEvent;
+ (void)trackInviteEventWithParams:(NSDictionary *)eventParams;

+ (void)trackRegistrationEvent;
+ (void)trackRegistrationEventWithParams:(NSDictionary *)eventParams;

#
+ (void)trackEvent:(NSString*)name;
+ (void)trackEvent:(NSString*)name eventParams:(NSDictionary*)eventParams;

@end
