// Copyright (c) 2014-present, Facebook, Inc. All rights reserved.
//
// You are hereby granted a non-exclusive, worldwide, royalty-free license to use,
// copy, modify, and distribute this software in source code or binary form for use
// in connection with the web services and APIs provided by Facebook.
//
// As with any software that integrates with the Facebook platform, your use of
// this software is subject to the Facebook Developer Principles and Policies
// [http://developers.facebook.com/policy/]. This copyright notice shall be
// included in all copies or substantial portions of the software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#import "FBSDKApplicationDelegate.h"
#import "FBSDKApplicationDelegate+Internal.h"

#import <objc/runtime.h>

#import "FBSDKAppEvents+Internal.h"
#import "FBSDKConstants.h"
#import "FBSDKDynamicFrameworkLoader.h"
#import "FBSDKError.h"
#import "FBSDKGateKeeperManager.h"
#import "FBSDKInternalUtility.h"
#import "FBSDKLogger.h"
#import "FBSDKServerConfiguration.h"
#import "FBSDKServerConfigurationManager.h"
#import "FBSDKSettings+Internal.h"
#import "FBSDKTimeSpentData.h"
#import "FBSDKUtility.h"

#if !TARGET_OS_TV
#import "FBSDKBoltsMeasurementEventListener.h"
#import "FBSDKContainerViewController.h"
#import "FBSDKProfile+Internal.h"
#endif

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_10_0

NSNotificationName const FBSDKApplicationDidBecomeActiveNotification = @"com.facebook.sdk.FBSDKApplicationDidBecomeActiveNotification";

#else

NSString *const FBSDKApplicationDidBecomeActiveNotification = @"com.facebook.sdk.FBSDKApplicationDidBecomeActiveNotification";

#endif

static NSString *const FBSDKAppLinkInboundEvent = @"fb_al_inbound";

@implementation FBSDKApplicationDelegate
{
  NSHashTable<id<FBSDKApplicationObserving>> *_applicationObservers;
  BOOL _isAppLaunched;
}

#pragma mark - Class Methods

+ (void)load
{
    // when the app becomes active by any means,  kick off the initialization.
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(initializeWithLaunchData:)
                                                 name:UIApplicationDidFinishLaunchingNotification
                                               object:nil];
}

// Initialize SDK listeners
// Don't call this function in any place else. It should only be called when the class is loaded.
+ (void)initializeWithLaunchData:(NSNotification *)note
{
    NSDictionary *launchData = note.userInfo;

    [[self sharedInstance] application:[UIApplication sharedApplication] didFinishLaunchingWithOptions:launchData];

#if !TARGET_OS_TV
    // Register Listener for Bolts measurement events
    [FBSDKBoltsMeasurementEventListener defaultListener];
#endif
    // Set the SourceApplication for time spent data. This is not going to update the value if the app has already launched.
    [FBSDKTimeSpentData setSourceApplication:launchData[UIApplicationLaunchOptionsSourceApplicationKey]
                                     openURL:launchData[UIApplicationLaunchOptionsURLKey]];
    // Register on UIApplicationDidEnterBackgroundNotification events to reset source application data when app backgrounds.
    [FBSDKTimeSpentData registerAutoResetSourceApplication];

    [FBSDKInternalUtility validateFacebookReservedURLSchemes];
    // Remove the observer
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

+ (instancetype)sharedInstance
{
    static FBSDKApplicationDelegate *_sharedInstance;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        _sharedInstance = [[self alloc] _init];
    });
    return _sharedInstance;
}

#pragma mark - Object Lifecycle

- (instancetype)_init
{
  if ((self = [super init]) != nil) {
    NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];
    [defaultCenter addObserver:self selector:@selector(applicationDidEnterBackground:) name:UIApplicationDidEnterBackgroundNotification object:nil];
    [defaultCenter addObserver:self selector:@selector(applicationDidBecomeActive:) name:UIApplicationDidBecomeActiveNotification object:nil];

    [[FBSDKAppEvents singleton] registerNotifications];
    _applicationObservers = [[NSHashTable alloc] init];
  }
  return self;
}

- (instancetype)init
{
    return nil;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

#pragma mark - UIApplicationDelegate

#if __IPHONE_OS_VERSION_MAX_ALLOWED > __IPHONE_9_0
- (BOOL)application:(UIApplication *)application
            openURL:(NSURL *)url
            options:(NSDictionary<UIApplicationOpenURLOptionsKey,id> *)options
{
    if (@available(iOS 9.0, *)) {
        return [self application:application
                         openURL:url
               sourceApplication:options[UIApplicationOpenURLOptionsSourceApplicationKey]
                      annotation:options[UIApplicationOpenURLOptionsAnnotationKey]];
    }

    return NO;
}
#endif

- (BOOL)application:(UIApplication *)application
            openURL:(NSURL *)url
  sourceApplication:(NSString *)sourceApplication
         annotation:(id)annotation
{
  if (sourceApplication != nil && ![sourceApplication isKindOfClass:[NSString class]]) {
    @throw [NSException exceptionWithName:NSInvalidArgumentException
                                   reason:@"Expected 'sourceApplication' to be NSString. Please verify you are passing in 'sourceApplication' from your app delegate (not the UIApplication* parameter). If your app delegate implements iOS 9's application:openURL:options:, you should pass in options[UIApplicationOpenURLOptionsSourceApplicationKey]. "
                                 userInfo:nil];
  }
  [FBSDKTimeSpentData setSourceApplication:sourceApplication openURL:url];

  BOOL handled = NO;
  NSArray<id<FBSDKApplicationObserving>> *observers = [_applicationObservers allObjects];
  for (id<FBSDKApplicationObserving> observer in observers) {
    if ([observer respondsToSelector:@selector(application:openURL:sourceApplication:annotation:)]) {
      if ([observer application:application
                        openURL:url
              sourceApplication:sourceApplication
                     annotation:annotation]) {
        handled = YES;
      }
    }
  }

  if (handled) {
    return YES;
  }

  [self _logIfAppLinkEvent:url];

  return NO;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    if ([self isAppLaunched]) {
        return NO;
    }

    _isAppLaunched = YES;
    FBSDKAccessToken *cachedToken = [FBSDKSettings accessTokenCache].accessToken;
    [FBSDKAccessToken setCurrentAccessToken:cachedToken];
    // fetch app settings
    [FBSDKServerConfigurationManager loadServerConfigurationWithCompletionBlock:NULL];
    // fetch gate keepers
    [FBSDKGateKeeperManager loadGateKeepers];

    if ([FBSDKSettings autoLogAppEventsEnabled].boolValue) {
        [self _logSDKInitialize];
    }
#if !TARGET_OS_TV
    FBSDKProfile *cachedProfile = [FBSDKProfile fetchCachedProfile];
    [FBSDKProfile setCurrentProfile:cachedProfile];
#endif
  NSArray<id<FBSDKApplicationObserving>> *observers = [_applicationObservers allObjects];
  BOOL handled = NO;
  for (id<FBSDKApplicationObserving> observer in observers) {
    if ([observer respondsToSelector:@selector(application:didFinishLaunchingWithOptions:)]) {
      if ([observer application:application didFinishLaunchingWithOptions:launchOptions]) {
        handled = YES;
      }
    }
  }

  return handled;
}

- (void)applicationDidEnterBackground:(NSNotification *)notification
{
  NSArray<id<FBSDKApplicationObserving>> *observers = [_applicationObservers allObjects];
  for (id<FBSDKApplicationObserving> observer in observers) {
    if ([observer respondsToSelector:@selector(applicationDidEnterBackground:)]) {
      [observer applicationDidEnterBackground:notification.object];
    }
  }
}

- (void)applicationDidBecomeActive:(NSNotification *)notification
{
  // Auto log basic events in case autoLogAppEventsEnabled is set
  if ([[FBSDKSettings autoLogAppEventsEnabled] boolValue]) {
    [FBSDKAppEvents activateApp];
  }

  NSArray<id<FBSDKApplicationObserving>> *observers = [_applicationObservers copy];
  for (id<FBSDKApplicationObserving> observer in observers) {
    if ([observer respondsToSelector:@selector(applicationDidBecomeActive:)]) {
      [observer applicationDidBecomeActive:notification.object];
    }
  }
}

#pragma mark - Internal Methods

- (void)addObserver:(id<FBSDKApplicationObserving>)observer
{
  if (![_applicationObservers containsObject:observer]) {
    [_applicationObservers addObject:observer];
  }
}

- (void)removeObserver:(id<FBSDKApplicationObserving>)observer
{
  if ([_applicationObservers containsObject:observer]) {
    [_applicationObservers removeObject:observer];
  }
}

#pragma mark - Helper Methods

- (void)_logIfAppLinkEvent:(NSURL *)url
{
    if (!url) {
        return;
    }
    NSDictionary *params = [FBSDKUtility dictionaryWithQueryString:url.query];
    NSString *applinkDataString = params[@"al_applink_data"];
    if (!applinkDataString) {
        return;
    }

    NSDictionary *applinkData = [FBSDKInternalUtility objectForJSONString:applinkDataString error:NULL];
    if (!applinkData) {
        return;
    }

    NSString *targetURLString = applinkData[@"target_url"];
    NSURL *targetURL = [targetURLString isKindOfClass:[NSString class]] ? [NSURL URLWithString:targetURLString] : nil;

    NSMutableDictionary *logData = [[NSMutableDictionary alloc] init];
    [FBSDKInternalUtility dictionary:logData setObject:targetURL.absoluteString forKey:@"targetURL"];
    [FBSDKInternalUtility dictionary:logData setObject:targetURL.host forKey:@"targetURLHost"];

    NSDictionary *refererData = applinkData[@"referer_data"];
    if (refererData) {
        [FBSDKInternalUtility dictionary:logData setObject:refererData[@"target_url"] forKey:@"referralTargetURL"];
        [FBSDKInternalUtility dictionary:logData setObject:refererData[@"url"] forKey:@"referralURL"];
        [FBSDKInternalUtility dictionary:logData setObject:refererData[@"app_name"] forKey:@"referralAppName"];
    }
    [FBSDKInternalUtility dictionary:logData setObject:url.absoluteString forKey:@"inputURL"];
    [FBSDKInternalUtility dictionary:logData setObject:url.scheme forKey:@"inputURLScheme"];

    [FBSDKAppEvents logImplicitEvent:FBSDKAppLinkInboundEvent
                          valueToSum:nil
                          parameters:logData
                         accessToken:nil];
}

- (void)_logSDKInitialize
{
    NSMutableDictionary *params = [NSMutableDictionary new];
    params[@"core_lib_included"] = @1;
    if (objc_lookUpClass("FBSDKShareDialog") != nil) {
        params[@"share_lib_included"] = @1;
    }
    if (objc_lookUpClass("FBSDKLoginManager") != nil) {
        params[@"login_lib_included"] = @1;
    }
    if (objc_lookUpClass("FBSDKPlacesManager") != nil) {
        params[@"places_lib_included"] = @1;
    }
    if (objc_lookUpClass("FBSDKMessengerButton") != nil) {
        params[@"messenger_lib_included"] = @1;
    }
    if (objc_lookUpClass("FBSDKMessengerButton") != nil) {
        params[@"messenger_lib_included"] = @1;
    }
    if (objc_lookUpClass("FBSDKTVInterfaceFactory.m") != nil) {
        params[@"tv_lib_included"] = @1;
    }
    if (objc_lookUpClass("FBSDKAutoLog") != nil) {
        params[@"marketing_lib_included"] = @1;
    }
    [FBSDKAppEvents logEvent:@"fb_sdk_initialize" parameters:params];
}

// Wrapping this makes it mockable and enables testability
- (BOOL)isAppLaunched {
  return _isAppLaunched;
}

@end
