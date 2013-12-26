//
//  MPAdConversionTracker.m
//  MoPub
//
//  Created by Andrew He on 2/4/11.
//  Copyright 2011 MoPub, Inc. All rights reserved.
//

#import "MPAdConversionTracker.h"
#import "MPConstants.h"
#import "MPGlobal.h"
#import "MPLogging.h"
#import "MPIdentityProvider.h"
#import "MPInstanceProvider.h"

#define MOPUB_CONVERSION_DEFAULTS_KEY @"com.mopub.conversion"

@interface MPAdConversionTracker ()

@property (nonatomic, retain) NSMutableData *responseData;
@property (nonatomic, assign) int statusCode;

- (NSURL *)URLForAppID:(NSString *)appID;

@end

@implementation MPAdConversionTracker

@synthesize responseData = _responseData;
@synthesize statusCode = _statusCode;

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

- (void)dealloc
{
    self.responseData = nil;
    [super dealloc];
}

- (void)reportApplicationOpenForApplicationID:(NSString *)appID
{
    if (![[NSUserDefaults standardUserDefaults] boolForKey:MOPUB_CONVERSION_DEFAULTS_KEY]) {
        MPLogInfo(@"Tracking conversion");
        NSMutableURLRequest *request = [[MPInstanceProvider sharedProvider] buildConfiguredURLRequestWithURL:[self URLForAppID:appID]];
        self.responseData = [NSMutableData data];
        [NSURLConnection connectionWithRequest:request delegate:self];
    }
}

#pragma mark - <NSURLConnectionDataDelegate>

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
    self.statusCode = [(NSHTTPURLResponse *)response statusCode];
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
    [self.responseData appendData:data];
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    //NOOP
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
    if (self.statusCode == 200 && [self.responseData length] > 0) {
        [[NSUserDefaults standardUserDefaults] setBool:YES forKey:MOPUB_CONVERSION_DEFAULTS_KEY];
        [[NSUserDefaults standardUserDefaults] synchronize];
    }
}

#pragma mark -
#pragma mark Internal

- (NSURL *)URLForAppID:(NSString *)appID
{
    NSString *path = [NSString stringWithFormat:@"http://%@/m/open?v=%@&udid=%@&id=%@&av=%@",
                      HOSTNAME,
                      MP_SERVER_VERSION,
                      [MPIdentityProvider identifier],
                      appID,
                      [[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"] stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]
                      ];

    return [NSURL URLWithString:path];
}
@end
