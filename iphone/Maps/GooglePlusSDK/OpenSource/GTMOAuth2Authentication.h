/* Copyright (c) 2011 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if GTM_INCLUDE_OAUTH2 || !GDATA_REQUIRE_SERVICE_INCLUDES

// This class implements the OAuth 2 protocol for authorizing requests.
// http://tools.ietf.org/html/draft-ietf-oauth-v2

#import <Foundation/Foundation.h>

// GTMHTTPFetcher.h brings in GTLDefines/GDataDefines
#import "GTMHTTPFetcher.h"

#undef _EXTERN
#undef _INITIALIZE_AS
#ifdef GTMOAUTH2AUTHENTICATION_DEFINE_GLOBALS
  #define _EXTERN
  #define _INITIALIZE_AS(x) =x
#else
  #if defined(__cplusplus)
    #define _EXTERN extern "C"
  #else
    #define _EXTERN extern
  #endif
  #define _INITIALIZE_AS(x)
#endif

// Until all OAuth 2 providers are up to the same spec, we'll provide a crude
// way here to override the "Bearer" string in the Authorization header
#ifndef GTM_OAUTH2_BEARER
#define GTM_OAUTH2_BEARER "Bearer"
#endif

// Service provider name allows stored authorization to be associated with
// the authorizing service
_EXTERN NSString* const kGTMOAuth2ServiceProviderGoogle _INITIALIZE_AS(@"Google");

//
// GTMOAuth2SignIn constants, included here for use by clients
//
_EXTERN NSString* const kGTMOAuth2ErrorDomain  _INITIALIZE_AS(@"com.google.GTMOAuth2");

// Error userInfo keys
_EXTERN NSString* const kGTMOAuth2ErrorMessageKey _INITIALIZE_AS(@"error");
_EXTERN NSString* const kGTMOAuth2ErrorRequestKey _INITIALIZE_AS(@"request");
_EXTERN NSString* const kGTMOAuth2ErrorJSONKey    _INITIALIZE_AS(@"json");

enum {
  // Error code indicating that the window was prematurely closed
  kGTMOAuth2ErrorWindowClosed          = -1000,
  kGTMOAuth2ErrorAuthorizationFailed   = -1001,
  kGTMOAuth2ErrorTokenExpired          = -1002,
  kGTMOAuth2ErrorTokenUnavailable      = -1003,
  kGTMOAuth2ErrorUnauthorizableRequest = -1004
};


// Notifications for token fetches
_EXTERN NSString* const kGTMOAuth2FetchStarted        _INITIALIZE_AS(@"kGTMOAuth2FetchStarted");
_EXTERN NSString* const kGTMOAuth2FetchStopped        _INITIALIZE_AS(@"kGTMOAuth2FetchStopped");

_EXTERN NSString* const kGTMOAuth2FetcherKey          _INITIALIZE_AS(@"fetcher");
_EXTERN NSString* const kGTMOAuth2FetchTypeKey        _INITIALIZE_AS(@"FetchType");
_EXTERN NSString* const kGTMOAuth2FetchTypeToken      _INITIALIZE_AS(@"token");
_EXTERN NSString* const kGTMOAuth2FetchTypeRefresh    _INITIALIZE_AS(@"refresh");
_EXTERN NSString* const kGTMOAuth2FetchTypeAssertion  _INITIALIZE_AS(@"assertion");
_EXTERN NSString* const kGTMOAuth2FetchTypeUserInfo   _INITIALIZE_AS(@"userInfo");

// Token-issuance errors
_EXTERN NSString* const kGTMOAuth2ErrorKey                  _INITIALIZE_AS(@"error");

_EXTERN NSString* const kGTMOAuth2ErrorInvalidRequest       _INITIALIZE_AS(@"invalid_request");
_EXTERN NSString* const kGTMOAuth2ErrorInvalidClient        _INITIALIZE_AS(@"invalid_client");
_EXTERN NSString* const kGTMOAuth2ErrorInvalidGrant         _INITIALIZE_AS(@"invalid_grant");
_EXTERN NSString* const kGTMOAuth2ErrorUnauthorizedClient   _INITIALIZE_AS(@"unauthorized_client");
_EXTERN NSString* const kGTMOAuth2ErrorUnsupportedGrantType _INITIALIZE_AS(@"unsupported_grant_type");
_EXTERN NSString* const kGTMOAuth2ErrorInvalidScope         _INITIALIZE_AS(@"invalid_scope");

// Notification that sign-in has completed, and token fetches will begin (useful
// for displaying interstitial messages after the window has closed)
_EXTERN NSString* const kGTMOAuth2UserSignedIn              _INITIALIZE_AS(@"kGTMOAuth2UserSignedIn");

// Notification for token changes
_EXTERN NSString* const kGTMOAuth2AccessTokenRefreshed _INITIALIZE_AS(@"kGTMOAuth2AccessTokenRefreshed");
_EXTERN NSString* const kGTMOAuth2RefreshTokenChanged  _INITIALIZE_AS(@"kGTMOAuth2RefreshTokenChanged");

// Notification for WebView loading
_EXTERN NSString* const kGTMOAuth2WebViewStartedLoading _INITIALIZE_AS(@"kGTMOAuth2WebViewStartedLoading");
_EXTERN NSString* const kGTMOAuth2WebViewStoppedLoading _INITIALIZE_AS(@"kGTMOAuth2WebViewStoppedLoading");
_EXTERN NSString* const kGTMOAuth2WebViewKey            _INITIALIZE_AS(@"kGTMOAuth2WebViewKey");
_EXTERN NSString* const kGTMOAuth2WebViewStopKindKey    _INITIALIZE_AS(@"kGTMOAuth2WebViewStopKindKey");
_EXTERN NSString* const kGTMOAuth2WebViewFinished       _INITIALIZE_AS(@"finished");
_EXTERN NSString* const kGTMOAuth2WebViewFailed         _INITIALIZE_AS(@"failed");
_EXTERN NSString* const kGTMOAuth2WebViewCancelled      _INITIALIZE_AS(@"cancelled");

// Notification for network loss during html sign-in display
_EXTERN NSString* const kGTMOAuth2NetworkLost         _INITIALIZE_AS(@"kGTMOAuthNetworkLost");
_EXTERN NSString* const kGTMOAuth2NetworkFound        _INITIALIZE_AS(@"kGTMOAuthNetworkFound");

@interface GTMOAuth2Authentication : NSObject <GTMFetcherAuthorizationProtocol>  {
 @private
  NSString *clientID_;
  NSString *clientSecret_;
  NSString *redirectURI_;
  NSMutableDictionary *parameters_;

  // authorization parameters
  NSURL *tokenURL_;
  NSDate *expirationDate_;

  NSDictionary *additionalTokenRequestParameters_;

  // queue of requests for authorization waiting for a valid access token
  GTMHTTPFetcher *refreshFetcher_;
  NSMutableArray *authorizationQueue_;

  id <GTMHTTPFetcherServiceProtocol> fetcherService_; // WEAK

  Class parserClass_;

  BOOL shouldAuthorizeAllRequests_;

  // arbitrary data retained for the user
  id userData_;
  NSMutableDictionary *properties_;
}

// OAuth2 standard protocol parameters
//
// These should be the plain strings; any needed escaping will be provided by
// the library.

// Request properties
@property (copy) NSString *clientID;
@property (copy) NSString *clientSecret;
@property (copy) NSString *redirectURI;
@property (retain) NSString *scope;
@property (retain) NSString *tokenType;
@property (retain) NSString *assertion;
@property (retain) NSString *refreshScope;

// Apps may optionally add parameters here to be provided to the token
// endpoint on token requests and refreshes
@property (retain) NSDictionary *additionalTokenRequestParameters;

// Response properties
@property (retain) NSMutableDictionary *parameters;

@property (retain) NSString *accessToken;
@property (retain) NSString *refreshToken;
@property (retain) NSNumber *expiresIn;
@property (retain) NSString *code;
@property (retain) NSString *errorString;

// URL for obtaining access tokens
@property (copy) NSURL *tokenURL;

// Calculated expiration date (expiresIn seconds added to the
// time the access token was received.)
@property (copy) NSDate *expirationDate;

// Service identifier, like "Google"; not used for authentication
//
// The provider name is just for allowing stored authorization to be associated
// with the authorizing service.
@property (copy) NSString *serviceProvider;

// User ID; not used for authentication
@property (retain) NSString *userID;

// User email and verified status; not used for authentication
//
// The verified string can be checked with -boolValue. If the result is false,
// then the email address is listed with the account on the server, but the
// address has not been confirmed as belonging to the owner of the account.
@property (retain) NSString *userEmail;
@property (retain) NSString *userEmailIsVerified;

// Property indicating if this auth has a refresh or access token so is suitable
// for authorizing a request. This does not guarantee that the token is valid.
@property (readonly) BOOL canAuthorize;

// Property indicating if this object will authorize plain http request
// (as well as any non-https requests.) Default is NO, only requests with the
// scheme https are authorized, since security may be compromised if tokens
// are sent over the wire using an unencrypted protocol like http.
@property (assign) BOOL shouldAuthorizeAllRequests;

// userData is retained for the convenience of the caller
@property (retain) id userData;

// Stored property values are retained for the convenience of the caller
@property (retain) NSDictionary *properties;

// Property for the optional fetcher service instance to be used to create
// fetchers
//
// Fetcher service objects retain authorizations, so this is weak to avoid
// circular retains.
@property (assign) id <GTMHTTPFetcherServiceProtocol> fetcherService; // WEAK

// Alternative JSON parsing class; this should implement the
// GTMOAuth2ParserClass informal protocol. If this property is
// not set, the class SBJSON must be available in the runtime.
@property (assign) Class parserClass;

// Convenience method for creating an authentication object
+ (id)authenticationWithServiceProvider:(NSString *)serviceProvider
                               tokenURL:(NSURL *)tokenURL
                            redirectURI:(NSString *)redirectURI
                               clientID:(NSString *)clientID
                           clientSecret:(NSString *)clientSecret;

// Clear out any authentication values, prepare for a new request fetch
- (void)reset;

// Main authorization entry points
//
// These will refresh the access token, if necessary, add the access token to
// the request, then invoke the callback.
//
// The request argument may be nil to just force a refresh of the access token,
// if needed.
//
// NOTE: To avoid accidental leaks of bearer tokens, the request must
// be for a URL with the scheme https unless the shouldAuthorizeAllRequests
// property is set.

// The finish selector should have a signature matching
//   - (void)authentication:(GTMOAuth2Authentication *)auth
//                  request:(NSMutableURLRequest *)request
//        finishedWithError:(NSError *)error;

- (void)authorizeRequest:(NSMutableURLRequest *)request
                delegate:(id)delegate
       didFinishSelector:(SEL)sel;

#if NS_BLOCKS_AVAILABLE
- (void)authorizeRequest:(NSMutableURLRequest *)request
       completionHandler:(void (^)(NSError *error))handler;
#endif

// Synchronous entry point; authorizing this way cannot refresh an expired
// access token
- (BOOL)authorizeRequest:(NSMutableURLRequest *)request;

// If the authentication is waiting for a refresh to complete, spin the run
// loop, discarding events, until the fetch has completed
//
// This is only for use in testing or in tools without a user interface.
- (void)waitForCompletionWithTimeout:(NSTimeInterval)timeoutInSeconds;


//////////////////////////////////////////////////////////////////////////////
//
// Internal properties and methods for use by GTMOAuth2SignIn
//

// Pending fetcher to get a new access token, if any
@property (retain) GTMHTTPFetcher *refreshFetcher;

// Check if a request is queued up to be authorized
- (BOOL)isAuthorizingRequest:(NSURLRequest *)request;

// Check if a request appears to be authorized
- (BOOL)isAuthorizedRequest:(NSURLRequest *)request;

// Stop any pending refresh fetch. This will also cancel the authorization
// for all fetch requests pending authorization.
- (void)stopAuthorization;

// Prevents authorization callback for a given request.
- (void)stopAuthorizationForRequest:(NSURLRequest *)request;

// OAuth fetch user-agent header value
- (NSString *)userAgent;

// Parse and set token and token secret from response data
- (void)setKeysForResponseString:(NSString *)str;
- (void)setKeysForResponseDictionary:(NSDictionary *)dict;

// Persistent token string for keychain storage
//
// We'll use the format "refresh_token=foo&serviceProvider=bar" so we can
// easily alter what portions of the auth data are stored
//
// Use these methods for serialization
- (NSString *)persistenceResponseString;
- (void)setKeysForPersistenceResponseString:(NSString *)str;

// method to begin fetching an access token, used by the sign-in object
- (GTMHTTPFetcher *)beginTokenFetchWithDelegate:(id)delegate
                              didFinishSelector:(SEL)finishedSel;

// Entry point to post a notification about a fetcher currently used for
// obtaining or refreshing a token; the sign-in object will also use this
// to indicate when the user's email address is being fetched.
//
// Fetch type constants are above under "notifications for token fetches"
- (void)notifyFetchIsRunning:(BOOL)isStarting
                     fetcher:(GTMHTTPFetcher *)fetcher
                        type:(NSString *)fetchType;

// Arbitrary key-value properties retained for the user
- (void)setProperty:(id)obj forKey:(NSString *)key;
- (id)propertyForKey:(NSString *)key;

//
// Utilities
//

+ (NSString *)encodedOAuthValueForString:(NSString *)str;

+ (NSString *)encodedQueryParametersForDictionary:(NSDictionary *)dict;

+ (NSDictionary *)dictionaryWithResponseString:(NSString *)responseStr;

+ (NSString *)scopeWithStrings:(NSString *)firsStr, ... NS_REQUIRES_NIL_TERMINATION;
@end

#endif // GTM_INCLUDE_OAUTH2 || !GDATA_REQUIRE_SERVICE_INCLUDES
