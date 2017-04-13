//
//  MPAnalyticsTracker.m
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPAnalyticsTracker.h"
#import "MPAdConfiguration.h"
#import "MPCoreInstanceProvider.h"
#import "MPLogging.h"

@interface MPAnalyticsTracker ()

- (NSURLRequest *)requestForURL:(NSURL *)URL;

@end

@implementation MPAnalyticsTracker

+ (MPAnalyticsTracker *)tracker
{
    return [[MPAnalyticsTracker alloc] init];
}

- (void)trackImpressionForConfiguration:(MPAdConfiguration *)configuration
{
    MPLogDebug(@"Tracking impression: %@", configuration.impressionTrackingURL);
    [NSURLConnection connectionWithRequest:[self requestForURL:configuration.impressionTrackingURL]
                                  delegate:nil];
}

- (void)trackClickForConfiguration:(MPAdConfiguration *)configuration
{
    MPLogDebug(@"Tracking click: %@", configuration.clickTrackingURL);
    [NSURLConnection connectionWithRequest:[self requestForURL:configuration.clickTrackingURL]
                                  delegate:nil];
}

- (void)sendTrackingRequestForURLs:(NSArray *)URLs
{
    for (NSURL *URL in URLs) {
        NSURLRequest *trackingRequest = [self requestForURL:URL];
        [NSURLConnection connectionWithRequest:trackingRequest delegate:nil];
    }
}

- (NSURLRequest *)requestForURL:(NSURL *)URL
{
    NSMutableURLRequest *request = [[MPCoreInstanceProvider sharedProvider] buildConfiguredURLRequestWithURL:URL];
    request.cachePolicy = NSURLRequestReloadIgnoringCacheData;
    return request;
}

@end
