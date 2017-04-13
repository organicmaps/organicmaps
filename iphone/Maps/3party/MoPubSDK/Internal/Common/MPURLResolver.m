//
//  MPURLResolver.m
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import <WebKit/WebKit.h>
#import "MPURLResolver.h"
#import "NSURL+MPAdditions.h"
#import "NSHTTPURLResponse+MPAdditions.h"
#import "MPInstanceProvider.h"
#import "MPLogging.h"
#import "MPCoreInstanceProvider.h"

static NSString * const kMoPubSafariScheme = @"mopubnativebrowser";
static NSString * const kMoPubSafariNavigateHost = @"navigate";
static NSString * const kResolverErrorDomain = @"com.mopub.resolver";

@interface MPURLResolver ()

@property (nonatomic, strong) NSURL *originalURL;
@property (nonatomic, strong) NSURL *currentURL;
@property (nonatomic, strong) NSURLConnection *connection;
@property (nonatomic, strong) NSMutableData *responseData;
@property (nonatomic, assign) NSStringEncoding responseEncoding;
@property (nonatomic, copy) MPURLResolverCompletionBlock completion;

- (MPURLActionInfo *)actionInfoFromURL:(NSURL *)URL error:(NSError **)error;
- (NSString *)storeItemIdentifierForURL:(NSURL *)URL;
- (BOOL)URLShouldOpenInApplication:(NSURL *)URL;
- (BOOL)URLIsHTTPOrHTTPS:(NSURL *)URL;
- (BOOL)URLPointsToAMap:(NSURL *)URL;
- (NSStringEncoding)stringEncodingFromContentType:(NSString *)contentType;

@end

@implementation MPURLResolver

@synthesize originalURL = _originalURL;
@synthesize currentURL = _currentURL;
@synthesize connection = _connection;
@synthesize responseData = _responseData;
@synthesize completion = _completion;

+ (instancetype)resolverWithURL:(NSURL *)URL completion:(MPURLResolverCompletionBlock)completion
{
    return [[MPURLResolver alloc] initWithURL:URL completion:completion];
}

- (instancetype)initWithURL:(NSURL *)URL completion:(MPURLResolverCompletionBlock)completion
{
    self = [super init];
    if (self) {
        _originalURL = [URL copy];
        _completion = [completion copy];
    }
    return self;
}

- (void)start
{
    [self.connection cancel];
    self.currentURL = self.originalURL;

    NSError *error = nil;
    MPURLActionInfo *info = [self actionInfoFromURL:self.originalURL error:&error];

    if (info) {
        [self safeInvokeAndNilCompletionBlock:info error:nil];
    } else if (error) {
        [self safeInvokeAndNilCompletionBlock:nil error:error];
    } else {
        NSURLRequest *request = [[MPCoreInstanceProvider sharedProvider] buildConfiguredURLRequestWithURL:self.originalURL];
        self.responseData = [NSMutableData data];
        self.responseEncoding = NSUTF8StringEncoding;
        self.connection = [NSURLConnection connectionWithRequest:request delegate:self];
    }
}

- (void)cancel
{
    [self.connection cancel];
    self.connection = nil;
    self.completion = nil;
}

- (void)safeInvokeAndNilCompletionBlock:(MPURLActionInfo *)info error:(NSError *)error
{
    if (self.completion != nil) {
        self.completion(info, error);
        self.completion = nil;
    }
}

#pragma mark - Handling Application/StoreKit URLs

/*
 * Parses the provided URL for actions to perform (opening StoreKit, opening Safari, etc.).
 * If the URL represents an action, this method will return an info object containing data that is
 * relevant to the suggested action.
 */
- (MPURLActionInfo *)actionInfoFromURL:(NSURL *)URL error:(NSError **)error;
{
    MPURLActionInfo *actionInfo = nil;

    if (URL == nil) {
        if (error) {
            *error = [NSError errorWithDomain:kResolverErrorDomain code:-1 userInfo:@{NSLocalizedDescriptionKey: @"URL is nil"}];
        }
        return nil;
    }

    if ([self storeItemIdentifierForURL:URL]) {
        actionInfo = [MPURLActionInfo infoWithURL:self.originalURL iTunesItemIdentifier:[self storeItemIdentifierForURL:URL] iTunesStoreFallbackURL:URL];
    } else if ([self URLHasDeeplinkPlusScheme:URL]) {
        MPEnhancedDeeplinkRequest *request = [[MPEnhancedDeeplinkRequest alloc] initWithURL:URL];
        if (request) {
            actionInfo = [MPURLActionInfo infoWithURL:self.originalURL enhancedDeeplinkRequest:request];
        } else {
            actionInfo = [MPURLActionInfo infoWithURL:self.originalURL deeplinkURL:URL];
        }
    } else if ([self safariURLForURL:URL]) {
        actionInfo = [MPURLActionInfo infoWithURL:self.originalURL safariDestinationURL:[NSURL URLWithString:[self safariURLForURL:URL]]];
    } else if ([URL mp_isMoPubShareScheme]) {
        actionInfo = [MPURLActionInfo infoWithURL:self.originalURL shareURL:URL];
    } else if ([self URLShouldOpenInApplication:URL]) {
        actionInfo = [MPURLActionInfo infoWithURL:self.originalURL deeplinkURL:URL];
    } else if ([URL.scheme isEqualToString:@"http"]) { // handle HTTP requests in particular to get around ATS settings
        // As a note: `appTransportSecuritySettings` returns what makes sense for the iOS version. I.e., if the device
        // is running iOS 8, this method will always return `MPATSSettingAllowsArbitraryLoads`. If the device is running
        // iOS 9, this method will never give us `MPATSSettingAllowsArbitraryLoadsInWebContent`. As a result, we don't
        // have to do OS checks here; we can just trust these settings.
        MPATSSetting settings = [[MPCoreInstanceProvider sharedProvider] appTransportSecuritySettings];

        if ((settings & MPATSSettingAllowsArbitraryLoads) != 0) { // opens as normal if ATS is disabled
            // don't do anything
        } else if ((settings & MPATSSettingAllowsArbitraryLoadsInWebContent) != 0) { // opens in WKWebView if ATS is disabled for arbitrary web content
            actionInfo = [MPURLActionInfo infoWithURL:self.originalURL
                                       webViewBaseURL:self.currentURL];
        } else { // opens in Mobile Safari if no other option is available
            actionInfo = [MPURLActionInfo infoWithURL:self.originalURL
                                 safariDestinationURL:self.currentURL];
        }
    }

    return actionInfo;
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

- (BOOL)URLHasDeeplinkPlusScheme:(NSURL *)URL
{
    return [[URL.scheme lowercaseString] isEqualToString:@"deeplink+"];
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

#pragma mark - Identifying NSStringEncoding from NSURLResponse Content-Type header

- (NSStringEncoding)stringEncodingFromContentType:(NSString *)contentType
{
    NSStringEncoding encoding = NSUTF8StringEncoding;

    if (![contentType length]) {
        MPLogWarn(@"Attempting to set string encoding from nil %@", kMoPubHTTPHeaderContentType);
        return encoding;
    }

    NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern:@"(?<=charset=)[^;]*" options:kNilOptions error:nil];

    NSTextCheckingResult *charsetResult = [regex firstMatchInString:contentType options:kNilOptions range:NSMakeRange(0, [contentType length])];
    if (charsetResult && charsetResult.range.location != NSNotFound) {
        NSString *charset = [contentType substringWithRange:[charsetResult range]];

        // ensure that charset is not deallocated early
        CFStringRef cfCharset = CFBridgingRetain(charset);
        CFStringEncoding cfEncoding = CFStringConvertIANACharSetNameToEncoding(cfCharset);
        CFBridgingRelease(cfCharset);

        if (cfEncoding == kCFStringEncodingInvalidId) {
            return encoding;
        }
        encoding = CFStringConvertEncodingToNSStringEncoding(cfEncoding);
    }

    return encoding;
}

#pragma mark - <NSURLConnectionDataDelegate>

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
    [self.responseData appendData:data];
}

- (NSURLRequest *)connection:(NSURLConnection *)connection willSendRequest:(NSURLRequest *)request redirectResponse:(NSURLResponse *)response
{
    // First, check to see if the redirect URL matches any of our suggested actions.
    NSError *error = nil;
    MPURLActionInfo *info = [self actionInfoFromURL:request.URL error:&error];

    if (info) {
        [connection cancel];
        [self safeInvokeAndNilCompletionBlock:info error:nil];
        return nil;
    } else {
        // The redirected URL didn't match any actions, so we should continue with loading the URL.
        self.currentURL = request.URL;
        return request;
    }
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
    NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;

    NSDictionary *headers = [httpResponse allHeaderFields];
    NSString *contentType = [headers objectForKey:kMoPubHTTPHeaderContentType];
    self.responseEncoding = [httpResponse stringEncodingFromContentType:contentType];
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
    NSString *responseString = [[NSString alloc] initWithData:self.responseData encoding:self.responseEncoding];
    MPURLActionInfo *info = [MPURLActionInfo infoWithURL:self.originalURL HTTPResponseString:responseString webViewBaseURL:self.currentURL];
    [self safeInvokeAndNilCompletionBlock:info error:nil];
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    [self safeInvokeAndNilCompletionBlock:nil error:error];
}

@end
