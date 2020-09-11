//
//  MPRateLimitManager.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface MPRateLimitManager : NSObject

// This is a singleton.
+ (instancetype)sharedInstance;

/**
 Current rate limit state for the given ad unit ID.
 @param adUnitId The ad unit ID to check rate limit state for
 @return @c YES if presently rate limited for this ad unit Id, @c NO otherwise
 */
- (BOOL)isRateLimitedForAdUnitId:(NSString *)adUnitId;

/**
 Sets rate limit state to rate limited. Automatically expires after @c milliseconds milliseconds. Rate limiting to 0 or
 negative milliseconds will result in no rate limit, but the number and reason will still be saved for later.
 @param adUnitId The ad unit ID to rate limit for
 @param milliseconds The number of milliseconds to rate limit for. If 0, no rate limit will be put into effect, but the number will still be saved for later
 @param reason The reason for the rate limit. This is copied directly from the server response. This parameter is optional.
 */
- (void)setRateLimitTimerWithAdUnitId:(NSString *)adUnitId milliseconds:(NSInteger)milliseconds reason:(NSString * _Nullable)reason;

/**
 Given an ad unit ID, returns the last millisecond rate limit value for that ad unit ID.
 @param adUnitId The ad unit ID to check last millisecond value for
 @return The number of milliseconds for the last rate limit, or 0 for first request or if the last request didn't rate limit
 */
- (NSUInteger)lastRateLimitMillisecondsForAdUnitId:(NSString *)adUnitId;

/**
 Given an ad unit ID, returns the last reason value for that ad unit ID.
 @param adUnitId The ad unit ID to check reason for
 @return The last reason given for this ad unit id, or @c nil for first request or if no reason was given
 */
- (NSString * _Nullable)lastRateLimitReasonForAdUnitId:(NSString *)adUnitId;

@end

NS_ASSUME_NONNULL_END
