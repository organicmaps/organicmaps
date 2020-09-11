//
//  MPRateLimitManager.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPRateLimitManager.h"
#import "MPRateLimitConfiguration.h"

@interface MPRateLimitManager ()

// Ad Unit IDs are used as keys; @c MPRateLimitConfiguration objects are used as values
@property (nonatomic, strong) NSMutableDictionary <NSString *, MPRateLimitConfiguration *> * configurationDictionary;

@end

@implementation MPRateLimitManager

+ (instancetype)sharedInstance {
    static MPRateLimitManager * sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[self alloc] init];
    });
    return sharedInstance;
}

- (instancetype)init {
    if (self = [super init]) {
        _configurationDictionary = [NSMutableDictionary dictionary];
    }

    return self;
}

- (void)setRateLimitTimerWithAdUnitId:(NSString *)adUnitId milliseconds:(NSInteger)milliseconds reason:(NSString *)reason {
    // Fast fail if @c adUnitId is @c nil
    if (adUnitId == nil) {
        return;
    }

    @synchronized (self) {
        // Make new configuration if one does not already exist for this ad unit ID
        if (self.configurationDictionary[adUnitId] == nil) {
            self.configurationDictionary[adUnitId] = [[MPRateLimitConfiguration alloc] init];
        }

        // Set the rate limit timer
        MPRateLimitConfiguration * config = self.configurationDictionary[adUnitId];
        [config setRateLimitTimerWithMilliseconds:milliseconds reason:reason];
    }
}

// Getter methods will return a default value upon a @c nil ad unit ID to avoid crashing on dictionary
// lookups. The return statement template is
// `return adUnitId != nil ? <configuration value from dictionary lookup> : <default value>`
// Using `!=` instead of `==` allows the configuration value to be listed first, then default second.

- (BOOL)isRateLimitedForAdUnitId:(NSString *)adUnitId {
    return adUnitId != nil ? self.configurationDictionary[adUnitId].isRateLimited : NO;
}

- (NSUInteger)lastRateLimitMillisecondsForAdUnitId:(NSString *)adUnitId {
    return adUnitId != nil ? self.configurationDictionary[adUnitId].lastRateLimitMilliseconds : 0;
}

- (NSString *)lastRateLimitReasonForAdUnitId:(NSString *)adUnitId {
    return adUnitId != nil ? self.configurationDictionary[adUnitId].lastRateLimitReason : nil;
}

@end
