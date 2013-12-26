//
//  MPURLResolver.m
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPURLResolver.h"
#import "NSURL+MPAdditions.h"
#import "MPInstanceProvider.h"

static NSString * const kMoPubSafariScheme = @"mopubnativebrowser";
static NSString * const kMoPubSafariNavigateHost = @"navigate";

#define kNumEncodingsToTry 2
static NSStringEncoding gEncodingWaterfall[kNumEncodingsToTry] = {NSUTF8StringEncoding, NSISOLatin1StringEncoding};

@interface MPURLResolver ()

@property (nonatomic, retain) NSURL *URL;
@property (nonatomic, retain) NSURLConnection *connection;
@property (nonatomic, retain) NSMutableData *responseData;

- (BOOL)handleURL:(NSURL *)URL;
- (NSString *)storeItemIdentifierForURL:(NSURL *)URL;
- (BOOL)URLShouldOpenInApplication:(NSURL *)URL;
- (BOOL)URLIsHTTPOrHTTPS:(NSURL *)URL;
- (BOOL)URLPointsToAMap:(NSURL *)URL;

@end

@implementation MPURLResolver

@synthesize URL = _URL;
@synthesize delegate = _delegate;
@synthesize connection = _connection;
@synthesize responseData = _responseData;

+ (MPURLResolver *)resolver
{
    return [[[MPURLResolver alloc] init] autorelease];
}

- (void)dealloc
{
    self.URL = nil;
    self.connection = nil;
    self.responseData = nil;

    [super dealloc];
}

- (void)startResolvingWithURL:(NSURL *)URL delegate:(id<MPURLResolverDelegate>)delegate
{
    [self.connection cancel];

    self.URL = URL;
    self.delegate = delegate;
    self.responseData = [NSMutableData data];

    if (![self handleURL:self.URL]) {
        NSURLRequest *request = [[MPInstanceProvider sharedProvider] buildConfiguredURLRequestWithURL:self.URL];
        self.connection = [NSURLConnection connectionWithRequest:request delegate:self];
    }
}

- (void)cancel
{
    [self.connection cancel];
    self.connection = nil;
}

- (NSString *)htmlStringForData:(NSData *)data
{
    NSString *htmlString = nil;

    for(int i = 0; i < kNumEncodingsToTry; i++)
    {
        htmlString = [[NSString alloc] initWithData:data encoding:gEncodingWaterfall[i]];
        if(htmlString != nil)
        {
            break;
        }
    }

    return [htmlString autorelease];
}

#pragma mark - Handling Application/StoreKit URLs

/*
 * Parses the provided URL for actions to perform (opening StoreKit, opening Safari, etc.).
 * If the URL represents an action, this method will inform its delegate of the correct action to
 * perform.
 *
 * Returns YES if the URL contained an action, and NO otherwise.
 */
- (BOOL)handleURL:(NSURL *)URL
{
    if ([self storeItemIdentifierForURL:URL]) {
        [self.delegate showStoreKitProductWithParameter:[self storeItemIdentifierForURL:URL] fallbackURL:URL];
    } else if ([self safariURLForURL:URL]) {
        NSURL *safariURL = [NSURL URLWithString:[self safariURLForURL:URL]];
        [self.delegate openURLInApplication:safariURL];
    } else if ([self URLShouldOpenInApplication:URL]) {
        if ([[UIApplication sharedApplication] canOpenURL:URL]) {
            [self.delegate openURLInApplication:URL];
        } else {
            [self.delegate failedToResolveURLWithError:[NSError errorWithDomain:@"com.mopub" code:-1 userInfo:nil]];
        }
    } else {
        return NO;
    }

    return YES;
}

#pragma mark Identifying Application URLs

- (BOOL)URLShouldOpenInApplication:(NSURL *)URL
{
    return ![self URLIsHTTPOrHTTPS:URL] || [self URLPointsToAMap:URL];
}

- (BOOL)URLIsHTTPOrHTTPS:(NSURL *)URL
{
    return [URL.scheme isEqualToString:@"http"] || [URL.scheme isEqualToString:@"https"];
}

- (BOOL)URLPointsToAMap:(NSURL *)URL
{
    return [URL.host hasSuffix:@"maps.google.com"] || [URL.host hasSuffix:@"maps.apple.com"];
}

#pragma mark Extracting StoreItem Identifiers

- (NSString *)storeItemIdentifierForURL:(NSURL *)URL
{
    NSString *itemIdentifier = nil;
    if ([URL.host hasSuffix:@"itunes.apple.com"]) {
        NSString *lastPathComponent = [[URL path] lastPathComponent];
        if ([lastPathComponent hasPrefix:@"id"]) {
            itemIdentifier = [lastPathComponent substringFromIndex:2];
        } else {
            itemIdentifier = [URL.mp_queryAsDictionary objectForKey:@"id"];
        }
    } else if ([URL.host hasSuffix:@"phobos.apple.com"]) {
        itemIdentifier = [URL.mp_queryAsDictionary objectForKey:@"id"];
    }

    NSCharacterSet *nonIntegers = [[NSCharacterSet decimalDigitCharacterSet] invertedSet];
    if (itemIdentifier && itemIdentifier.length > 0 && [itemIdentifier rangeOfCharacterFromSet:nonIntegers].location == NSNotFound) {
        return itemIdentifier;
    }

    return nil;
}

#pragma mark - Identifying URLs to open in Safari

- (NSString *)safariURLForURL:(NSURL *)URL
{
    NSString *safariURL = nil;

    if ([[URL scheme] isEqualToString:kMoPubSafariScheme] &&
        [[URL host] isEqualToString:kMoPubSafariNavigateHost]) {
        safariURL = [URL.mp_queryAsDictionary objectForKey:@"url"];
    }

    return safariURL;
}

#pragma mark - <NSURLConnectionDataDelegate>

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
    [self.responseData appendData:data];
}

- (NSURLRequest *)connection:(NSURLConnection *)connection willSendRequest:(NSURLRequest *)request redirectResponse:(NSURLResponse *)response
{
    if ([self handleURL:request.URL]) {
        [connection cancel];
        return nil;
    } else {
        self.URL = request.URL;
        return request;
    }
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
    [self.delegate showWebViewWithHTMLString:[self htmlStringForData:self.responseData]
                                     baseURL:self.URL];
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    [self.delegate failedToResolveURLWithError:error];
}

@end
