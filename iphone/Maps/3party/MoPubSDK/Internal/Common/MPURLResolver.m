//
//  MPURLResolver.m
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import <WebKit/WebKit.h>
#import "MPURLResolver.h"
#import "MPHTTPNetworkSession.h"
#import "NSURL+MPAdditions.h"
#import "NSHTTPURLResponse+MPAdditions.h"
#import "MPInstanceProvider.h"
#import "MPLogging.h"
#import "MPCoreInstanceProvider.h"
#import "MOPUBExperimentProvider.h"
#import "NSURL+MPAdditions.h"
#import "MPURLRequest.h"

static NSString * const kMoPubSafariScheme = @"mopubnativebrowser";
static NSString * const kMoPubSafariNavigateHost = @"navigate";
static NSString * const kResolverErrorDomain = @"com.mopub.resolver";
static NSString * const kWebviewClickthroughHost = @"ads.mopub.com";
static NSString * const kWebviewClickthroughPath = @"/m/aclk";
static NSString * const kRedirectURLQueryStringKey = @"r";

@interface MPURLResolver ()

@property (nonatomic, strong) NSURL *originalURL;
@property (nonatomic, strong) NSURL *currentURL;
@property (nonatomic, strong) NSURLSessionTask *task;
@property (nonatomic, copy) MPURLResolverCompletionBlock completion;

- (MPURLActionInfo *)actionInfoFromURL:(NSURL *)URL error:(NSError **)error;
- (NSString *)storeItemIdentifierForURL:(NSURL *)URL;
- (BOOL)URLShouldOpenInApplication:(NSURL *)URL;
- (BOOL)URLIsHTTPOrHTTPS:(NSURL *)URL;
- (BOOL)URLPointsToAMap:(NSURL *)URL;
- (NSStringEncoding)stringEncodingFromContentType:(NSString *)contentType;

@end

@implementation MPURLResolver

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
    [self.task cancel];
    self.currentURL = self.originalURL;

    NSError *error = nil;
    MPURLActionInfo *info = [self actionInfoFromURL:self.originalURL error:&error];

    if (info) {
        [self safeInvokeAndNilCompletionBlock:info error:nil];
    } else if ([self shouldEnableClickthroughExperiment]) {
        info = [MPURLActionInfo infoWithURL:self.originalURL webViewBaseURL:self.currentURL];
        [self safeInvokeAndNilCompletionBlock:info error:nil];
    } else if (error) {
        [self safeInvokeAndNilCompletionBlock:nil error:error];
    } else {
        MPURLRequest *request = [[MPURLRequest alloc] initWithURL:self.originalURL];
        self.task = [self httpTaskWithRequest:request];
    }
}

- (NSURLSessionTask *)httpTaskWithRequest:(MPURLRequest *)request {
    __weak __typeof__(self) weakSelf = self;
    NSURLSessionTask * task = [MPHTTPNetworkSession startTaskWithHttpRequest:request responseHandler:^(NSData * _Nonnull data, NSHTTPURLResponse * _Nonnull response) {
        __typeof__(self) strongSelf = weakSelf;

        // Set the response content type
        NSStringEncoding responseEncoding = NSUTF8StringEncoding;
        NSDictionary *headers = [response allHeaderFields];
        NSString *contentType = [headers objectForKey:kMoPubHTTPHeaderContentType];
        if (contentType != nil) {
            responseEncoding = [response stringEncodingFromContentType:contentType];
        }

        NSString *responseString = [[NSString alloc] initWithData:data encoding:responseEncoding];
        MPURLActionInfo *info = [MPURLActionInfo infoWithURL:strongSelf.originalURL HTTPResponseString:responseString webViewBaseURL:strongSelf.currentURL];
        [strongSelf safeInvokeAndNilCompletionBlock:info error:nil];

    } errorHandler:^(NSError * _Nonnull error) {
        __typeof__(self) strongSelf = weakSelf;
        [strongSelf safeInvokeAndNilCompletionBlock:nil error:error];
    } shouldRedirectWithNewRequest:^BOOL(NSURLSessionTask * _Nonnull task, NSURLRequest * _Nonnull newRequest) {
        __typeof__(self) strongSelf = weakSelf;

        // First, check to see if the redirect URL matches any of our suggested actions.
        NSError * actionInfoError = nil;
        MPURLActionInfo * info = [strongSelf actionInfoFromURL:newRequest.URL error:&actionInfoError];

        if (info) {
            [task cancel];
            [strongSelf safeInvokeAndNilCompletionBlock:info error:nil];
            return NO;
        } else {
            // The redirected URL didn't match any actions, so we should continue with loading the URL.
            strongSelf.currentURL = newRequest.URL;
            return YES;
        }
    }];

    return task;
}

- (void)cancel
{
    [self.task cancel];
    self.task = nil;
    self.completion = nil;
}

- (void)safeInvokeAndNilCompletionBlock:(MPURLActionInfo *)info error:(NSError *)error
{
    dispatch_async(dispatch_get_main_queue(), ^{
        if (self.completion != nil) {
            self.completion(info, error);
            self.completion = nil;
        }
    });
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
        CFStringRef cfCharset = (CFStringRef)CFBridgingRetain(charset);
        CFStringEncoding cfEncoding = CFStringConvertIANACharSetNameToEncoding(cfCharset);
        CFBridgingRelease(cfCharset);

        if (cfEncoding == kCFStringEncodingInvalidId) {
            return encoding;
        }
        encoding = CFStringConvertEncodingToNSStringEncoding(cfEncoding);
    }

    return encoding;
}

#pragma mark - Check if it's necessary to include a URL in the clickthrough experiment.
// There are two types of clickthrough URL sources: from webviews and from non-web views.
// The ones from webviews start with (https|http)://ads.mopub.com/m/aclk
// For webviews, in order for a URL to be included in the clickthrough experiment, redirect URL scheme needs to be http/https.

- (BOOL)shouldEnableClickthroughExperiment
{
    if (!self.currentURL) {
        return NO;
    }

    // If redirect URL isn't http/https, do not include it in the clickthrough experiment.
    if (![self URLIsHTTPOrHTTPS:self.currentURL]) {
        return NO;
    }

    // Clickthroughs from webviews
    if ([self.currentURL.host isEqualToString:kWebviewClickthroughHost] &&
        [self.currentURL.path isEqualToString:kWebviewClickthroughPath]) {

        NSString *redirectURLStr = [self.currentURL mp_queryParameterForKey:kRedirectURLQueryStringKey];
        if (!redirectURLStr || ![self URLIsHTTPOrHTTPS:[NSURL URLWithString:redirectURLStr]]) {
            return NO;
        }
    }

    // Check experiment variant is in test group.
    if ([MPAdDestinationDisplayAgent shouldUseSafariViewController] ||
            [MOPUBExperimentProvider displayAgentType] == MOPUBDisplayAgentTypeNativeSafari) {
        return YES;
    }
    return NO;
}

@end
