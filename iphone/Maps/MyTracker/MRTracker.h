//
// Created by Igor Glotov on 22/07/14.
// Copyright (c) 2014 Mailru Group. All rights reserved.
// MyTracker, version 1.1.1

#import <Foundation/Foundation.h>

@interface MRTracker : NSObject

+ (BOOL)debugMode;
+ (void)setDebugMode:(BOOL)enable;

- (void)setup;
- (BOOL)isInitialized;

- (void)trackEvent:(NSString *)name;
- (void)trackEvent:(NSString *)name eventParams:(NSDictionary *)eventParams;

- (void)trackLoginEvent;
- (void)trackLoginEventWithParams:(NSDictionary *)eventParams;

- (void)trackInviteEvent;
- (void)trackInviteEventWithParams:(NSDictionary *)eventParams;

- (void)trackRegistrationEvent;
- (void)trackRegistrationEventWithParams:(NSDictionary *)eventParams;

@end
