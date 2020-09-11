//
//  MPRateLimitConfiguration.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface MPRateLimitConfiguration : NSObject

/**
 Returns the number of milliseconds of the last rate limit, or 0 for first request or if the last request was not rate limited.
 */
@property (nonatomic, readonly) NSUInteger lastRateLimitMilliseconds;

/**
 Returns the reason for the last rate limit, or nil for first request or if the last request did not include a reason.
 */
@property (nonatomic, copy, readonly, nullable) NSString * lastRateLimitReason;

/**
 Returns present rate limit state. @c YES if presently rate limited, @c NO otherwise
 */
@property (nonatomic, readonly) BOOL isRateLimited;

/**
 Sets rate limit state to rate limited. Automatically expires after @c milliseconds milliseconds. Rate limiting to 0 or
 negative milliseconds will result in no rate limit, but the number and reason will still be saved for later.
 @param milliseconds The number of milliseconds to rate limit for. If 0, no rate limit will be put into effect, but the number will still be saved for later
 @param reason The reason for the rate limit. This is copied directly from the server response. This parameter is optional.
 */
- (void)setRateLimitTimerWithMilliseconds:(NSInteger)milliseconds reason:(NSString * _Nullable)reason;

@end

NS_ASSUME_NONNULL_END
