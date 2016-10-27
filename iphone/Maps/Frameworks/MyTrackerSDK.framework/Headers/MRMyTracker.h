//
//  MRMyTracker.h
//  myTrackerSDK 1.5.3
//
//  Created by Timur Voloshin on 17.06.16.
//  Copyright © 2016 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>

@class MRMyTrackerParams;

@interface MRMyTracker : NSObject

+ (void)setDebugMode:(BOOL)enabled;

+ (BOOL)isDebugMode;

+ (void)setEnabled:(BOOL)enabled;

+ (BOOL)isEnabled;

+ (NSString *)trackerVersion;

+ (MRMyTrackerParams *)trackerParams;

+ (void)createTracker:(NSString *)trackerId;

+ (void)setupTracker;

+ (void)trackEventWithName:(NSString *)name;

+ (void)trackEventWithName:(NSString *)name eventParams:(NSDictionary<NSString *, NSString *> *)eventParams;

+ (void)trackLoginEvent;

+ (void)trackLoginEventWithParams:(NSDictionary<NSString *, NSString *> *)eventParams;

+ (void)trackInviteEvent;

+ (void)trackInviteEventWithParams:(NSDictionary<NSString *, NSString *> *)eventParams;

+ (void)trackRegistrationEvent;

+ (void)trackRegistrationEventWithParams:(NSDictionary<NSString *, NSString *> *)eventParams;

+ (void)trackPurchaseWithProduct:(id)product transaction:(id)transaction;

+ (void)trackPurchaseWithProduct:(id)product transaction:(id)transaction eventParams:(NSDictionary<NSString *, NSString *> *)eventParams;

+ (void)trackLevelAchieved;

+ (void)trackLevelAchievedWithLevel:(NSNumber *)level;

+ (void)trackLevelAchievedWithLevel:(NSNumber *)level eventParams:(NSDictionary<NSString *, NSString *> *)eventParams;

@end
