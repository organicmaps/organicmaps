//
//  MPIdentityProvider.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

@interface MPIdentityProvider : NSObject

/**
 * Return IDFA if it's available. If IDFA is not available or contains only 0s, return MoPub rotation ID that changes every 24 hours.
 */
+ (NSString *)identifier;

/**
 * Return IDFA if it's available. If IDFA is not available or contains only 0s, return nil.
 */
+ (NSString *)identifierFromASIdentifierManager:(BOOL)obfuscate;

/**
* Return MoPub UUID
*/
+ (NSString *)obfuscatedIdentifier;

/**
 * Return the unobfuscated MoPub UUID, without the "mopub:" prefix.
 */
+ (NSString *)unobfuscatedMoPubIdentifier;

+ (BOOL)advertisingTrackingEnabled;

/**
 * A Boolean value indicating whether the MoPub SDK should create a MoPub ID that can be used
 * for frequency capping when Limit ad tracking is on & the IDFA we get is
 * 00000000-0000-0000-0000-000000000000.
 *
 * When set to NO, the SDK will not create a MoPub ID in the above case. When set to YES, the
 * SDK will generate a MoPub ID. The default value is YES.
 *
 */
+ (void)setFrequencyCappingIdUsageEnabled:(BOOL)frequencyCappingIdUsageEnabled;
+ (BOOL)frequencyCappingIdUsageEnabled;

@end
