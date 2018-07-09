//
//  MPURLRequest.m
//  MoPubSDK
//
//  Copyright © 2018 MoPub. All rights reserved.
//

#import "MPURLRequest.h"
#import "MPAPIEndpoints.h"
#import "MPLogging.h"

// All requests have a 10 second timeout.
const NSTimeInterval kRequestTimeoutInterval = 10.0;

NS_ASSUME_NONNULL_BEGIN
@interface MPURLRequest()

@end

@implementation MPURLRequest

- (instancetype)initWithURL:(NSURL *)URL {
    // Requests sent to MoPub should always be in POST format. All other requests
    // should be sent as a normal GET.
    BOOL isMoPubRequest = [URL.host isEqualToString:MOPUB_BASE_HOSTNAME];
    NSURL * requestUrl = URL;
    if (isMoPubRequest) {
        // The incoming URL may contain query parameters; we will need to strip them out.
        NSURLComponents * components = [[NSURLComponents alloc] init];
        components.scheme = URL.scheme;
        components.host = URL.host;
        components.path = URL.path;
        requestUrl = components.URL;
    }

    if (self = [super initWithURL:requestUrl]) {
        // Generate the request
        [self setHTTPShouldHandleCookies:NO];
        [self setValue:MPURLRequest.userAgent forHTTPHeaderField:@"User-Agent"];
        [self setCachePolicy:NSURLRequestReloadIgnoringCacheData];
        [self setTimeoutInterval:kRequestTimeoutInterval];

        // Request is a MoPub specific request and should be sent as POST with a UTF8 JSON payload.
        if (isMoPubRequest) {
            [self setHTTPMethod:@"POST"];
            [self setValue:@"application/json; charset=utf-8" forHTTPHeaderField:@"Content-Type"];

            // Generate the JSON body from the query parameters
            NSURLComponents * components = [NSURLComponents componentsWithURL:URL resolvingAgainstBaseURL:NO];
            NSDictionary * json = [MPURLRequest jsonFromURLComponents:components];

            NSError * error = nil;
            NSData * jsonData = [NSJSONSerialization dataWithJSONObject:json options:0 error:&error];

            // Set the request body with the query parameter key/value pairs if there was no
            // error in generating a JSON from the dictionary.
            if (error == nil) {
                [self setValue:[NSString stringWithFormat:@"%lu", (unsigned long)jsonData.length] forHTTPHeaderField:@"Content-Length"];
                [self setHTTPBody:jsonData];
            }
            else {
                MPLogError(@"Could not generate JSON body for %@", json);
            }
        }
    }

    return self;
}

+ (MPURLRequest *)requestWithURL:(NSURL *)URL {
    return [[MPURLRequest alloc] initWithURL:URL];
}

/**
 Retrieves the current user agent as determined by @c UIWebView.
 @returns The user agent.
 */
+ (NSString *)userAgent {
    static NSString * ua = nil;

    if (ua == nil) {
        ua = [[[UIWebView alloc] init] stringByEvaluatingJavaScriptFromString:@"navigator.userAgent"];
    }

    return ua;
}

/**
 Generates the POST body as a JSON dictionary. The keys to the dictionary
 are the query parameter keys, and the values are the associated values.
 In the event that there are multiple keys present, they will be combined into
 a comma-seperated list string.
 @remark The values will be URL-decoded before being set in the JSON dictionary
 @param components URL components to generate the JSON
 @returns A JSON dictionary
 */
+ (NSDictionary *)jsonFromURLComponents:(NSURLComponents *)components {
    NSMutableDictionary * json = [NSMutableDictionary new];

    // If there are no components, just give back an empty JSON
    if (components == nil) {
        return json;
    }

    // Iterate over every query parameter and rationalize them into
    // the JSON dictionary.
    for (NSURLQueryItem * queryItem in components.queryItems) {
        NSString * key = queryItem.name;
        NSString * decodedValue = [queryItem.value stringByRemovingPercentEncoding];
        decodedValue = decodedValue != nil ? decodedValue : @"";

        if ([json objectForKey:key] != nil) {
            json[key] = [@[json[key], decodedValue] componentsJoinedByString:@","];
        }
        // Key doesn't exist; add it.
        else {
            json[key] = decodedValue;
        }
    }

    return json;
}

@end
NS_ASSUME_NONNULL_END
