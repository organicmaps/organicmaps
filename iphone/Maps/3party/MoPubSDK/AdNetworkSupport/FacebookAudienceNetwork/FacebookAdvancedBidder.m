//
//  FacebookAdvancedBidder.m
//  MoPubSDK
//
//  Copyright © 2017 MoPub. All rights reserved.
//

#import "FacebookAdvancedBidder.h"
#import <FBAudienceNetwork/FBAudienceNetwork.h>

@implementation FacebookAdvancedBidder

+ (void)initialize {
    if (self == [FacebookAdvancedBidder self]) {
        // Initialize an adview to trigger [FBAdUtility initializeAudienceNetwork]
        FBAdView * initHackforFacebook = [[FBAdView alloc] initWithPlacementID:@"" adSize:kFBAdSize320x50 rootViewController:[UIViewController new]];
        if (initHackforFacebook) {
            NSLog(@"Initialized Facebook Audience Network");
        }
    }
}

- (NSString *)creativeNetworkName {
    return @"facebook";
}

- (NSString *)token {
    return [FBAdSettings bidderToken];
}

@end
