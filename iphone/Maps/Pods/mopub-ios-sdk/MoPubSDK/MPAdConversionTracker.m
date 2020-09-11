//
//  MPAdConversionTracker.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPAdConversionTracker.h"
#import "MPConstants.h"
#import "MPGlobal.h"
#import "MPLogging.h"
#import "MPIdentityProvider.h"
#import "MPCoreInstanceProvider.h"
#import "MPAPIEndpoints.h"
#import "MPHTTPNetworkSession.h"
#import "MPURLRequest.h"
#import "MPConsentManager.h"
#import "MPAdServerURLBuilder.h"

@interface MPAdConversionTracker ()
@property (nonatomic, strong) NSURLSessionTask * task;
@end

@implementation MPAdConversionTracker

+ (MPAdConversionTracker *)sharedConversionTracker
{
    static MPAdConversionTracker *sharedConversionTracker;

    @synchronized(self)
    {
        if (!sharedConversionTracker)
            sharedConversionTracker = [[MPAdConversionTracker alloc] init];
        return sharedConversionTracker;
    }
}


- (void)reportApplicationOpenForApplicationID:(NSString *)appID
{
    // Store app ID in case retry is needed.
    [[NSUserDefaults standardUserDefaults] setObject:appID forKey:MOPUB_CONVERSION_APP_ID_KEY];
    [[NSUserDefaults standardUserDefaults] synchronize];

    // Do not send app conversion request if collecting personal information is not allowed.
    if (![MPConsentManager sharedManager].canCollectPersonalInfo) {
        return;
    }

    if (![[NSUserDefaults standardUserDefaults] boolForKey:MOPUB_CONVERSION_DEFAULTS_KEY]) {
        MPLogInfo(@"Tracking conversion");
        MPURLRequest * request = [[MPURLRequest alloc] initWithURL:[MPAdServerURLBuilder conversionTrackingURLForAppID:appID]];
        self.task = [MPHTTPNetworkSession startTaskWithHttpRequest:request responseHandler:^(NSData * data, NSHTTPURLResponse * response) {
            if (response.statusCode == 200 && data.length > 0) {
                [[NSUserDefaults standardUserDefaults] removeObjectForKey:MOPUB_CONVERSION_APP_ID_KEY];
                [[NSUserDefaults standardUserDefaults] setBool:YES forKey:MOPUB_CONVERSION_DEFAULTS_KEY];
                [[NSUserDefaults standardUserDefaults] synchronize];
            }
        } errorHandler:nil];
    }
}

@end
