//
//  MPSessionTracker.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPSessionTracker.h"
#import "MPConstants.h"
#import "MPHTTPNetworkSession.h"
#import "MPURLRequest.h"
#import "MPAdServerURLBuilder.h"

@implementation MPSessionTracker

+ (void)initializeNotificationObservers
{
    if (SESSION_TRACKING_ENABLED) {
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(trackEvent)
                                                     name:UIApplicationWillEnterForegroundNotification
                                                   object:nil];

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(trackEvent)
                                                     name:UIApplicationDidFinishLaunchingNotification
                                                   object:nil];
    }
}

+ (void)trackEvent
{
    MPURLRequest * request = [[MPURLRequest alloc] initWithURL:[MPAdServerURLBuilder sessionTrackingURL]];
    [MPHTTPNetworkSession startTaskWithHttpRequest:request responseHandler:nil errorHandler:nil];
}

@end
