//
//  MPAnalyticsTracker.m
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPAnalyticsTracker.h"
#import "MPAdConfiguration.h"
#import "MPCoreInstanceProvider.h"
#import "MPHTTPNetworkSession.h"
#import "MPLogging.h"
#import "MPURLRequest.h"

@implementation MPAnalyticsTracker

+ (MPAnalyticsTracker *)tracker
{
    return [[MPAnalyticsTracker alloc] init];
}

- (void)trackImpressionForConfiguration:(MPAdConfiguration *)configuration
{
    MPLogDebug(@"Tracking impression: %@", configuration.impressionTrackingURL);
    MPURLRequest * request = [[MPURLRequest alloc] initWithURL:configuration.impressionTrackingURL];
    [MPHTTPNetworkSession startTaskWithHttpRequest:request];
}

- (void)trackClickForConfiguration:(MPAdConfiguration *)configuration
{
    MPLogDebug(@"Tracking click: %@", configuration.clickTrackingURL);
    MPURLRequest * request = [[MPURLRequest alloc] initWithURL:configuration.clickTrackingURL];
    [MPHTTPNetworkSession startTaskWithHttpRequest:request];
}

- (void)sendTrackingRequestForURLs:(NSArray *)URLs
{
    for (NSURL *URL in URLs) {
        MPURLRequest * trackingRequest = [[MPURLRequest alloc] initWithURL:URL];
        [MPHTTPNetworkSession startTaskWithHttpRequest:trackingRequest];
    }
}

@end
