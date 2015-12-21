//
//  MRMyTracker.h
//  myTrackerSDKCorp 1.3.2
//
//  Created by Igor Glotov on 22.07.14.
//  Copyright © 2014 Mail.ru Group. All rights reserved.
//


#import <Foundation/Foundation.h>
#import <MyTrackerSDKCorp/MRTracker.h>

extern NSString * const MYTRACKER_VERSION_STRING;

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

//SKProduct*
//SKPaymentTransaction*
+ (void) trackPurchaseWithProduct:(id)product transaction:(id)transaction;
+ (void) trackPurchaseWithProduct:(id)product transaction:(id)transaction eventParams:(NSDictionary *)eventParams;

+ (void) trackLevelAchieved;
+ (void) trackLevelAchieved:(NSNumber*)level;
+ (void) trackLevelAchieved:(NSNumber*)level eventParams:(NSDictionary *)eventParams;

+ (void)trackEvent:(NSString*)name;
+ (void)trackEvent:(NSString*)name eventParams:(NSDictionary*)eventParams;

@end
