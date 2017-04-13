//
//  MPFacebookAttributionIdProvider.h
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPKeywordProvider.h"

/*
 * This class enables the MoPub SDK to deliver targeted ads from Facebook via MoPub Marketplace
 * (MoPub's real-time bidding ad exchange) as part of a test program. This class sends an identifier
 * to Facebook so Facebook can select the ad MoPub will serve in your app through MoPub Marketplace.
 * If this class is removed from the SDK, your application will not receive targeted ads from
 * Facebook.
 */

@interface MPFacebookKeywordProvider : NSObject <MPKeywordProvider>

@end
