//
//  MPSessionTracker.m
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPSessionTracker.h"
#import "MPConstants.h"
#import "MPIdentityProvider.h"
#import "MPGlobal.h"
#import "MPCoreInstanceProvider.h"
#import "MPAPIEndpoints.h"
#import "MPHTTPNetworkSession.h"
#import "MPURLRequest.h"
#import "MPAdServerURLBuilder.h"

@implementation MPSessionTracker

+ (void)load
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
