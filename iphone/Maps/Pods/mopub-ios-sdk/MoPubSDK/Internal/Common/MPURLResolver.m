//
//  MPURLResolver.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <StoreKit/StoreKit.h>
#import <WebKit/WebKit.h>
#import "MPURLResolver.h"
#import "MPHTTPNetworkSession.h"
#import "NSURL+MPAdditions.h"
#import "NSHTTPURLResponse+MPAdditions.h"
#import "MPLogging.h"
#import "MPDeviceInformation.h"
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
    } else if ([self shouldOpenWithInAppWebBrowser]) {
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

    NSDictionary * storeKitParameters = [self appStoreProductParametersForURL:URL];
    if (storeKitParameters != nil) {
        actionInfo = [MPURLActionInfo infoWithURL:self.originalURL iTunesStoreParameters:storeKitParameters iTunesStoreFallbackURL:URL];
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
        MPATSSetting settings = MPDeviceInformation.appTransportSecuritySettings;

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

- (BOOL)URLIsAppleScheme:(NSURL *)URL
{
    // Definitely not an Apple URL scheme.
    if (![URL.host hasSuffix:@".apple.com"]) {
        return NO;
    }

    // Constant set of supported Apple Store subdomains that will be loaded into
    // SKStoreProductViewController. This is lazily initialized and limited to the
    // scope of this method.
    static NSSet * supportedStoreSubdomains = nil;
    if (supportedStoreSubdomains == nil) {
        supportedStoreSubdomains = [NSSet setWithArray:@[@"apps", @"books", @"itunes", @"music"]];
    }

    // Assumes that the Apple Store sub domains are of the format store-type.apple.com
    // At this point we are guaranteed at least 3 components from the previous ".apple.com"
    // check.
    NSArray * hostComponents = [URL.host componentsSeparatedByString:@"."];
    NSString * subdomain = hostComponents[0];

    return [supportedStoreSubdomains containsObject:subdomain];
}

#pragma mark Extracting StoreItem Identifiers

/**
 Attempt to parse an Apple store URL into a dictionary of @c SKStoreProductParameter items. This will fast fail
 if the URL is not a valid Apple store URL scheme.
 @param URL Apple store URL to attempt to parse.
 @return A dictionary with at least the required @c SKStoreProductParameterITunesItemIdentifier as an entry; otherwise @c nil
 */
- (NSDictionary *)appStoreProductParametersForURL:(NSURL *)URL
{
    // Definitely not an Apple URL scheme. Don't bother to parse.
    if (![self URLIsAppleScheme:URL]) {
        return nil;
    }

    // Failed to parse out the URL into its components. Likely to be an invalid URL.
    NSURLComponents * urlComponents = [NSURLComponents componentsWithURL:URL resolvingAgainstBaseURL:YES];
    if (urlComponents == nil) {
        return nil;
    }

    // Attempt to parse out the item identifier.
    NSString * itemIdentifier = ({
        NSString * lastPathComponent = URL.path.lastPathComponent;
        NSString * itemIdFromQueryParameter = [URL.mp_queryAsDictionary objectForKey:@"id"];
        NSString * parsedIdentifier = nil;

        // Old style iTunes item identifiers are prefixed with "id".
        // Example: https://apps.apple.com/.../id923917775
        if ([lastPathComponent hasPrefix:@"id"]) {
            parsedIdentifier = [lastPathComponent substringFromIndex:2];
        }
        // Look for the item identifier as a query parameter in the URL.
        // Example: https://itunes.apple.com/...?id=923917775
        else if (itemIdFromQueryParameter != nil) {
            parsedIdentifier = itemIdFromQueryParameter;
        }
        // Newer style Apple Store identifiers are just the last path component.
        // Example: https://music.apple.com/.../1451047660
        else {
            parsedIdentifier = lastPathComponent;
        }

        // Check that the parsed item identifier doesn't exist or contains invalid characters.
        NSCharacterSet * nonIntegers = [[NSCharacterSet decimalDigitCharacterSet] invertedSet];
        if (parsedIdentifier.length > 0 && [parsedIdentifier rangeOfCharacterFromSet:nonIntegers].location != NSNotFound) {
            parsedIdentifier = nil;
        }

        parsedIdentifier;
    });

    // Item identifier is a required field. If it doesn't exist, there is no point
    // in continuing to parse the URL.
    if (itemIdentifier.length == 0) {
        return nil;
    }

    // Attempt parsing for the following StoreKit product keys:
    // SKStoreProductParameterITunesItemIdentifier      (required)
    // SKStoreProductParameterProductIdentifier         (not supported)
    // SKStoreProductParameterAdvertisingPartnerToken   (not supported)
    // SKStoreProductParameterAffiliateToken            (optional)
    // SKStoreProductParameterCampaignToken             (optional)
    // SKStoreProductParameterProviderToken             (not supported)
    //
    // Query parameter parsing according to:
    // https://affiliate.itunes.apple.com/resources/documentation/basic_affiliate_link_guidelines_for_the_phg_network/
    NSMutableDictionary * parameters = [NSMutableDictionary dictionaryWithCapacity:3];
    parameters[SKStoreProductParameterITunesItemIdentifier] = itemIdentifier;

    for (NSURLQueryItem * queryParameter in urlComponents.queryItems) {
        // OPTIONAL: Attempt parsing of SKStoreProductParameterAffiliateToken
        if ([queryParameter.name isEqualToString:@"at"]) {
            parameters[SKStoreProductParameterAffiliateToken] = queryParameter.value;
        }
        // OPTIONAL: Attempt parsing of SKStoreProductParameterCampaignToken
        else if ([queryParameter.name isEqualToString:@"ct"]) {
            parameters[SKStoreProductParameterCampaignToken] = queryParameter.value;
        }
    }

    return parameters;
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
        MPLogInfo(@"Attempting to set string encoding from nil %@", kMoPubHTTPHeaderContentType);
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

#pragma mark - Check if it's necessary to handle the clickthrough URL outside of a web browser
// There are two types of clickthrough URL sources: from webviews and from non-web views.
// The ones from webviews start with (https|http)://ads.mopub.com/m/aclk
// For webviews, in order for a URL to be processed in a web browser, the redirect URL scheme needs to be http/https.
- (BOOL)shouldOpenWithInAppWebBrowser
{
    if (!self.currentURL) {
        return NO;
    }

    // If redirect URL isn't http/https, do not open it in a browser. It is likely a deep link
    // or an Apple Store scheme that will need special parsing.
    if (![self URLIsHTTPOrHTTPS:self.currentURL] || [self URLIsAppleScheme:self.currentURL]) {
        return NO;
    }

    // Clickthroughs from webviews
    if ([self.currentURL.host isEqualToString:kWebviewClickthroughHost] &&
        [self.currentURL.path isEqualToString:kWebviewClickthroughPath]) {

        // Extract the redirect URL from the clickthrough.
        NSString *redirectURLStr = [self.currentURL mp_queryParameterForKey:kRedirectURLQueryStringKey];
        NSURL *redirectUrl = [NSURL URLWithString:redirectURLStr];

        // There is a redirect URL. We need to determine if the redirect also needs additional
        // handling. In the event that no redirect URL is present, normal processing will occur.
        if (redirectUrl != nil && (![self URLIsHTTPOrHTTPS:redirectUrl] || [self URLIsAppleScheme:redirectUrl])) {
            return NO;
        }
    }

    // ADF-4215: If this trailing return value should be changed, check whether App Store redirection
    // links will end up showing the App Store UI in app (expected) or escaping the app to open the
    // native iOS App Store (unexpected).
    return [MPAdDestinationDisplayAgent shouldDisplayContentInApp];
}

@end
