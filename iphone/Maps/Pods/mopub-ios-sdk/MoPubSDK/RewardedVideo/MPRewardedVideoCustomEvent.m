//
//  MPRewardedVideoCustomEvent.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPRewardedVideoCustomEvent.h"
#import <UIKit/UIKit.h>

@implementation MPRewardedVideoCustomEvent

- (void)requestRewardedVideoWithCustomEventInfo:(NSDictionary *)info adMarkup:(NSString *)adMarkup
{
    // The default implementation of this method does nothing. Subclasses must override this method
    // and implement code to load a rewarded video here.
}

- (BOOL)hasAdAvailable
{
    // Subclasses must override this method and implement coheck whether or not a rewarded vidoe ad
    // is available for presentation.

    return NO;
}

- (void)presentRewardedVideoFromViewController:(UIViewController *)viewController
{
    // The default implementation of this method does nothing. Subclasses must override this method
    // and implement code to display a rewarded video here.
}

- (BOOL)enableAutomaticImpressionAndClickTracking
{
    // Subclasses may override this method to return NO to perform impression and click tracking
    // manually.
    return YES;
}

- (void)handleAdPlayedForCustomEventNetwork
{
    // The default implementation of this method does nothing. Subclasses must override this method
    // and implement code to handle when another ad unit plays an ad for the same ad network this custom event is representing.
}

- (void)handleCustomEventInvalidated
{
    // The default implementation of this method does nothing. Subclasses must override this method
    // and implement code to handle when the custom event is no longer needed by the rewarded video system.
}

@end
