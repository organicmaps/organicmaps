//
//  MPAdServerURLBuilder.h
//  MoPub
//
//  Copyright (c) 2012 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

@interface MPAdServerURLBuilder : NSObject

+ (NSURL *)URLWithAdUnitID:(NSString *)adUnitID
                  keywords:(NSString *)keywords
          userDataKeywords:(NSString *)userDataKeywords
                  location:(CLLocation *)location;

+ (NSURL *)URLWithAdUnitID:(NSString *)adUnitID
                  keywords:(NSString *)keywords
          userDataKeywords:(NSString *)userDataKeywords
                  location:(CLLocation *)location
             desiredAssets:(NSArray *)assets
               viewability:(BOOL)viewability;

+ (NSURL *)URLWithAdUnitID:(NSString *)adUnitID
                  keywords:(NSString *)keywords
          userDataKeywords:(NSString *)userDataKeywords
                  location:(CLLocation *)location
             desiredAssets:(NSArray *)assets
                adSequence:(NSInteger)adSequence
               viewability:(BOOL)viewability;

@end

@interface MPAdServerURLBuilder (OpenEndpoint)

/**
 Constructs the conversion tracking URL using current consent state, SDK state, and @c appID parameter.
 @param appID The App ID to be included in the URL.
 @returns URL to the open endpoint configuring for conversion tracking.
 */
+ (NSURL *)conversionTrackingURLForAppID:(NSString *)appID;

/**
 Constructs the session tracking URL using current consent state and SDK state.
 @returns URL to the open endpoint configuring for session tracking.
 */
+ (NSURL *)sessionTrackingURL;

@end

@interface MPAdServerURLBuilder (Consent)

/**
 Constructs the consent synchronization endpoint URL using the current consent manager
 state.
 @returns URL to the consent synchronization endpoint.
 */
+ (NSURL *)consentSynchronizationUrl;

/**
 Constructs the URL to fetch the consent dialog using the current consent manager state.
 @returns URL to the consent dialog endpoint
 */
+ (NSURL *)consentDialogURL;

@end
