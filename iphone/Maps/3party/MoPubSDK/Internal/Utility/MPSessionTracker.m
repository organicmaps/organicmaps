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
    [NSURLConnection connectionWithRequest:[[MPCoreInstanceProvider sharedProvider] buildConfiguredURLRequestWithURL:[self URL]]
                                  delegate:nil];
}

+ (NSURL *)URL
{
    NSString *path = [NSString stringWithFormat:@"%@?v=%@&udid=%@&id=%@&av=%@&st=1",
                      [MPAPIEndpoints baseURLStringWithPath:MOPUB_API_PATH_SESSION testing:NO],
                      MP_SERVER_VERSION,
                      [MPIdentityProvider identifier],
                      [[[NSBundle mainBundle] bundleIdentifier] stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding],
                      [[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"] stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]
                      ];

    return [NSURL URLWithString:path];
}

@end
