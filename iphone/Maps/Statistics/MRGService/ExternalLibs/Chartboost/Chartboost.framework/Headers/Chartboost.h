/*
 * Chartboost.h
 * Chartboost
 * 5.0.3
 *
 * Copyright 2011 Chartboost. All rights reserved.
 */

/*!
 @typedef NS_ENUM (NSUInteger, CBFramework)
 
 @abstract
 Used with setFramework:(CBFramework)framework calls to set suffix for
 wrapper libraries like Unity or Corona.
 */
typedef NS_ENUM(NSUInteger, CBFramework) {
    /*! Unity. */
    CBFrameworkUnity,
    /*! Corona. */
    CBFrameworkCorona,
    /*! Adobe AIR. */
    CBFrameworkAIR,
    /*! GameSalad. */
    CBFrameworkGameSalad
};

/*!
 @typedef NS_ENUM (NSUInteger, CBLoadError)
 
 @abstract
 Returned to ChartboostDelegate methods to notify of Chartboost SDK errors.
 */
typedef NS_ENUM(NSUInteger, CBLoadError) {
    /*! Unknown internal error. */
    CBLoadErrorInternal,
    /*! Network is currently unavailable. */
    CBLoadErrorInternetUnavailable,
    /*! Too many requests are pending for that location.  */
    CBLoadErrorTooManyConnections,
    /*! Interstitial loaded with wrong orientation. */
    CBLoadErrorWrongOrientation,
    /*! Interstitial disabled, first session. */
    CBLoadErrorFirstSessionInterstitialsDisabled,
    /*! Network request failed. */
    CBLoadErrorNetworkFailure,
    /*!  No ad received. */
    CBLoadErrorNoAdFound,
    /*! Session not started. */
    CBLoadErrorSessionNotStarted,
    /*! User manually cancelled the impression. */
    CBLoadErrorUserCancellation,
    /*! No location detected. */
    CBLoadErrorNoLocationFound,
};

/*!
 @typedef NS_ENUM (NSUInteger, CBClickError)
 
 @abstract
 Returned to ChartboostDelegate methods to notify of Chartboost SDK errors.
 */
typedef NS_ENUM(NSUInteger, CBClickError) {
    /*! Invalid URI. */
    CBClickErrorUriInvalid,
    /*! The device does not know how to open the protocol of the URI  */
    CBClickErrorUriUnrecognized,
    /*! User failed to pass the age gate. */
    CBClickErrorAgeGateFailure,
    /*! Unknown internal error */
    CBClickErrorInternal,
};

/*!
 @typedef CBLocation
 
 @abstract
 Defines standard locations to describe where Chartboost SDK features appear in game.
 
 @discussion Standard locations used to describe where Chartboost features show up in your game
 For best performance, it is highly recommended to use standard locations.

 Benefits include:
 - Higher eCPMs.
 - Control of ad targeting and frequency.
 - Better reporting.
 */
typedef NSString * const CBLocation;

/*! "Startup" - Initial startup of game. */
extern CBLocation const CBLocationStartup;
/*! "Home Screen" - Home screen the player first sees. */
extern CBLocation const CBLocationHomeScreen;
/*! "Main Menu" - Menu that provides game options. */
extern CBLocation const CBLocationMainMenu;
/*! "Game Screen" - Game screen where all the magic happens. */
extern CBLocation const CBLocationGameScreen;
/*! "Achievements" - Screen with list of achievements in the game. */
extern CBLocation const CBLocationAchievements;
/*! "Quests" - Quest, missions or goals screen describing things for a player to do. */
extern CBLocation const CBLocationQuests;
/*!  "Pause" - Pause screen. */
extern CBLocation const CBLocationPause;
/*! "Level Start" - Start of the level. */
extern CBLocation const CBLocationLevelStart;
/*! "Level Complete" - Completion of the level */
extern CBLocation const CBLocationLevelComplete;
/*! "Turn Complete" - Finishing a turn in a game. */
extern CBLocation const CBLocationTurnComplete;
/*! "IAP Store" - The store where the player pays real money for currency or items. */
extern CBLocation const CBLocationIAPStore;
/*! "Item Store" - The store where a player buys virtual goods. */
extern CBLocation const CBLocationItemStore;
/*! "Game Over" - The game over screen after a player is finished playing. */
extern CBLocation const CBLocationGameOver;
/*! "Leaderboard" - List of leaders in the game. */
extern CBLocation const CBLocationLeaderBoard;
/*! "Settings" - Screen where player can change settings such as sound. */
extern CBLocation const CBLocationSettings;
/*! "Quit" - Screen displayed right before the player exits a game. */
extern CBLocation const CBLocationQuit;
/*! "Default" - Supports legacy applications that only have one "Default" location */
extern CBLocation const CBLocationDefault;

@protocol ChartboostDelegate;

/*!
 @class Chartboost
 
 @abstract
 Provide methods to display and control Chartboost native advertising types.
 
 @discussion For more information on integrating and using the Chartboost SDK
 please visit our help site documentation at https://help.chartboost.com
 */
@interface Chartboost : NSObject

#pragma mark - Main Chartboost API

/*!
 @abstract
 Start Chartboost with required appId, appSignature and delegate.

 @param appId The Chartboost application ID for this application.

 @param appSignature The Chartboost application signature for this application.

 @param delegate The delegate instance to receive Chartboost SDK callbacks.

 @discussion This method must be executed before any other Chartboost SDK methods can be used.
 Once executed this call will also controll session tracking and background tasks
 used by Chartboost.
*/
+ (void)startWithAppId:(NSString*)appId
          appSignature:(NSString*)appSignature
              delegate:(id<ChartboostDelegate>)delegate;

/*!
 @abstract
 Determine if a locally cached interstitial exists for the given CBLocation.
 
 @param location The location for the Chartboost impression type.
 
 @return YES if there a locally cached interstitial, and NO if not.
 
 @discussion A return value of YES here indicates that the corresponding
 showInterstitial:(CBLocation)location method will present without making
 additional Chartboost API server requests to fetch data to present.
 */
+ (BOOL)hasInterstitial:(CBLocation)location;

/*!
 @abstract
 Present an interstitial for the given CBLocation.
 
 @param location The location for the Chartboost impression type.
 
 @discussion This method will first check if there is a locally cached interstitial
 for the given CBLocation and, if found, will present using the locally cached data.
 If no locally cached data exists the method will attempt to fetch data from the
 Chartboost API server and present it.  If the Chartboost API server is unavailable
 or there is no eligible interstitial to present in the given CBLocation this method
 is a no-op.
 */
+ (void)showInterstitial:(CBLocation)location;

/*!
 @abstract
 Determine if a locally cached "more applications" exists for the given CBLocation.
 
 @param location The location for the Chartboost impression type.
 
 @return YES if there a locally cached "more applications", and NO if not.
 
 @discussion A return value of YES here indicates that the corresponding
 showMoreApps:(CBLocation)location method will present without making
 additional server requests to fetch data to present.
 */
+ (BOOL)hasMoreApps:(CBLocation)location;

/*!
 @abstract
 Present an "more applications" for the given CBLocation.
 
 @param location The location for the Chartboost impression type.
 
 @discussion This method will first check if there is a locally cached "more applications"
 for the given CBLocation and, if found, will present using the locally cached data.
 If no locally cached data exists the method will attempt to fetch data from the
 Chartboost API server and present it.  If the Chartboost API server is unavailable
 or there is no eligible "more applications" to present in the given CBLocation this method
 is a no-op.
 */
+ (void)showMoreApps:(CBLocation)location;

/*!
 @abstract
 Present an "more applications" for the given CBLocation and inside the given UIViewController.
 
 @param viewController The UIViewController to display the "more applications" UI within.
 
 @param location The location for the Chartboost impression type.
 
 @discussion This method uses the same implementation logic as showMoreApps:(CBLocation)location 
 for loading the "more applications" data, but adds an optional viewController parameter. 
 The viewController object allows the "more applications" page to be presented modally in a specified
 view hierarchy. If the Chartboost API server is unavailable or there is no eligible "more applications" 
 to present in the given CBLocation this method is a no-op.
 */
+ (void)showMoreApps:(UIViewController *)viewController
            location:(CBLocation)location;

/*!
 @abstract
 Determine if a locally cached rewarded video exists for the given CBLocation.
 
 @param location The location for the Chartboost impression type.
 
 @return YES if there a locally cached rewarded video, and NO if not.
 
 @discussion A return value of YES here indicates that the corresponding
 showRewardedVideo:(CBLocation)location method will present without making
 additional Chartboost API server requests to fetch data to present.
 */
+ (BOOL)hasRewardedVideo:(CBLocation)location;

/*!
 @abstract
 Present a rewarded video for the given CBLocation.
 
 @param location The location for the Chartboost impression type.
 
 @discussion This method will first check if there is a locally cached rewarded video
 for the given CBLocation and, if found, will present it using the locally cached data.
 If no locally cached data exists the method will attempt to fetch data from the
 Chartboost API server and present it.  If the Chartboost API server is unavailable
 or there is no eligible rewarded video to present in the given CBLocation this method
 is a no-op.
 */
+ (void)showRewardedVideo:(CBLocation)location;

#pragma mark - Advanced Configuration & Use

/*!
 @abstract
 Confirm if an age gate passed or failed. When specified Chartboost will wait for 
 this call before showing the IOS App Store.
 
 @param pass The result of successfully passing the age confirmation.
 
 @discussion If you have configured your Chartboost experience to use the age gate feature
 then this method must be executed after the user has confirmed their age.  The Chartboost SDK
 will halt until this is done.
 */
+ (void)didPassAgeGate:(BOOL)pass;

/*!
 @abstract
 Opens a "deep link" URL for a Chartboost Custom Scheme.
 
 @param url The URL to open.
 
 @param sourceApplication The application that originated the action.
 
 @return YES if Chartboost SDK is capable of handling the URL and does so, and NO if not.
 
 @discussion If you have configured a custom scheme and provided "deep link" URLs that the
 Chartboost SDK is capable of handling you should use this method in your ApplicationDelegate
 class methods that handle custom URL schemes.
 */
+ (BOOL)handleOpenURL:(NSURL *)url
    sourceApplication:(NSString *)sourceApplication;

/*!
 @abstract
 Opens a "deep link" URL for a Chartboost Custom Scheme.
 
 @param url The URL to open.
 
 @param sourceApplication The application that originated the action.
 
 @param annotation The provided annotation.
 
 @return YES if Chartboost SDK is capable of handling the URL and does so, and NO if not.
 
 @discussion If you have configured a custom scheme and provided "deep link" URLs that the
 Chartboost SDK is capable of handling you should use this method in your ApplicationDelegate
 class methods that handle custom URL schemes.
 */
+ (BOOL)handleOpenURL:(NSURL *)url
    sourceApplication:(NSString *)sourceApplication
           annotation:(id)annotation;

/*!
 @abstract
 Set a custom identifier to send in the POST body for all Chartboost API server requests.
 
 @param customId The identifier to send with all Chartboost API server requests.
 
 @discussion Use this method to set a custom identifier that can be used later in the Chartboost
 dashboard to group information by.
 */
+ (void)setCustomId:(NSString *)customId;

/*!
 @abstract
 Get the current custom identifier being sent in the POST body for all Chartboost API server requests.
 
 @return The identifier being sent with all Chartboost API server requests.
 
 @discussion Use this method to get the custom identifier that can be used later in the Chartboost
 dashboard to group information by.
 */
+ (NSString *)getCustomId;

/*!
 @abstract
 Set a custom framework suffix to append to the POST headers field.
 
 @param framework The suffx to send with all Chartboost API server requests.
 
 @discussion This is an internal method used via Chartboost's Unity and Corona SDKs
 to track their usage.
 */
+ (void)setFramework:(CBFramework)framework;

/*!
 @abstract
 Decide if Chartboost SDK should show interstitials in the first session.
 
 @param shouldRequest YES if allowed to show interstitials in first session, NO otherwise.
 
 @discussion Set to control if Chartboost SDK can show interstitials in the first session.
 The session count is controlled via the startWithAppId:appSignature:delegate: method in the Chartboost
 class.
 
 Default is YES.
 */
+ (void)setShouldRequestInterstitialsInFirstSession:(BOOL)shouldRequest;

/*!
 @abstract
 Decide if Chartboost SDK should block for an age gate.
 
 @param shouldPause YES if Chartboost should pause for an age gate, NO otherwise.
 
 @discussion Set to control if Chartboost SDK should block for an age gate.
 
 Default is NO.
 */
+ (void)setShouldPauseClickForConfirmation:(BOOL)shouldPause;

/*!
 @abstract
 Decide if Chartboost SDK should show a loading view while preparing to display the
 "more applications" UI.
 
 @param shouldDisplay YES if Chartboost should display a loading view, NO otherwise.
 
 @discussion Set to control if Chartboost SDK should show a loading view while
 preparing to display the "more applications" UI.
 
 Default is NO.
 */
+ (void)setShouldDisplayLoadingViewForMoreApps:(BOOL)shouldDisplay;

/*!
 @abstract
 Decide if Chartboost SDKK will attempt to fetch videos from the Chartboost API servers.
 
 @param shouldPrefetch YES if Chartboost should prefetch video content, NO otherwise.
 
 @discussion Set to control if Chartboost SDK control if videos should be prefetched.
 
 Default is YES.
 */
+ (void)setShouldPrefetchVideoContent:(BOOL)shouldPrefetch;

#pragma mark - Advanced Caching

/*!
 @abstract
 Cache an interstitial at the given CBLocation.
 
 @param location The location for the Chartboost impression type.
 
 @discussion This method will first check if there is a locally cached interstitial
 for the given CBLocation and, if found, will do nothing. If no locally cached data exists 
 the method will attempt to fetch data from the Chartboost API server.
 */
+ (void)cacheInterstitial:(CBLocation)location;

/*!
 @abstract
 Cache an "more applications" at the given CBLocation.
 
 @param location The location for the Chartboost impression type.
 
 @discussion This method will first check if there is a locally cached "more applications"
 for the given CBLocation and, if found, will do nothing. If no locally cached data exists
 the method will attempt to fetch data from the Chartboost API server.
 */
+ (void)cacheMoreApps:(CBLocation)location;

/*!
 @abstract
 Cache a rewarded video at the given CBLocation.
 
 @param location The location for the Chartboost impression type.
 
 @discussion This method will first check if there is a locally cached rewarded video
 for the given CBLocation and, if found, will do nothing. If no locally cached data exists
 the method will attempt to fetch data from the Chartboost API server.
 */
+ (void)cacheRewardedVideo:(CBLocation)location;

/*!
 @abstract
 Set to enable and disable the auto cache feature (Enabled by default).
 
 @param shouldCache The param to enable or disable auto caching.
 
 @discussion If set to YES the Chartboost SDK will automatically attempt to cache an impression
 once one has been consumed via a "show" call.  If set to NO, it is the responsibility of the
 developer to manage the caching behavior of Chartboost impressions.
 */
+ (void)setAutoCacheAds:(BOOL)shouldCache;

/*!
 @abstract
 Get the current auto cache behavior (Enabled by default).
 
 @return YES if the auto cache is enabled, NO if it is not.
 
 @discussion If set to YES the Chartboost SDK will automatically attempt to cache an impression
 once one has been consumed via a "show" call.  If set to NO, it is the responsibility of the
 developer to manage the caching behavior of Chartboost impressions.
 */
+ (BOOL)getAutoCacheAds;

@end

/*!
 @protocol ChartboostDelegate
 
 @abstract
 Provide methods and callbacks to receive notifications of when the Chartboost SDK
 has taken specific actions or to more finely control the Chartboost SDK.
 
 @discussion For more information on integrating and using the Chartboost SDK
 please visit our help site documentation at https://help.chartboost.com
 
 All of the delegate methods are optional.
 */
@protocol ChartboostDelegate <NSObject>

@optional

#pragma mark - Interstitial Delegate

/*!
 @abstract
 Called before requesting an interstitial via the Chartboost API server.
 
 @param location The location for the Chartboost impression type.
 
 @return YES if execution should proceed, NO if not.
 
 @discussion Implement to control if the Charboost SDK should fetch data from
 the Chartboost API servers for the given CBLocation.  This is evaluated
 if the showInterstitial:(CBLocation) or cacheInterstitial:(CBLocation)location
 are called.  If YES is returned the operation will proceed, if NO, then the
 operation is treated as a no-op.
 
 Default return is YES.
 */
- (BOOL)shouldRequestInterstitial:(CBLocation)location;

/*!
 @abstract
 Called before an interstitial will be displayed on the screen.
 
 @param location The location for the Chartboost impression type.
 
 @return YES if execution should proceed, NO if not.
 
 @discussion Implement to control if the Charboost SDK should display an interstitial
 for the given CBLocation.  This is evaluated if the showInterstitial:(CBLocation)
 is called.  If YES is returned the operation will proceed, if NO, then the
 operation is treated as a no-op and nothing is displayed.
 
 Default return is YES.
 */
- (BOOL)shouldDisplayInterstitial:(CBLocation)location;

/*!
 @abstract
 Called after an interstitial has been displayed on the screen.
 
 @param location The location for the Chartboost impression type.
 
 @discussion Implement to be notified of when an interstitial has
 been displayed on the screen for a given CBLocation.
 */
- (void)didDisplayInterstitial:(CBLocation)location;

/*!
 @abstract
 Called after an interstitial has been loaded from the Chartboost API
 servers and cached locally.
 
 @param location The location for the Chartboost impression type.
 
 @discussion Implement to be notified of when an interstitial has been loaded from the Chartboost API
 servers and cached locally for a given CBLocation.
 */
- (void)didCacheInterstitial:(CBLocation)location;

/*!
 @abstract
 Called after an interstitial has attempted to load from the Chartboost API
 servers but failed.
 
 @param location The location for the Chartboost impression type.
 
 @param error The reason for the error defined via a CBLoadError.
 
 @discussion Implement to be notified of when an interstitial has attempted to load from the Chartboost API
 servers but failed for a given CBLocation.
 */
- (void)didFailToLoadInterstitial:(CBLocation)location
                        withError:(CBLoadError)error;

/*!
 @abstract
 Called after a click is registered, but the user is not fowrwarded to the IOS App Store.
 
 @param location The location for the Chartboost impression type.
 
 @param error The reason for the error defined via a CBLoadError.
 
 @discussion Implement to be notified of when a click is registered, but the user is not fowrwarded 
 to the IOS App Store for a given CBLocation.
 */
- (void)didFailToRecordClick:(CBLocation)location
                   withError:(CBClickError)error;

/*!
 @abstract
 Called after an interstitial has been dismissed.
 
 @param location The location for the Chartboost impression type.
 
 @discussion Implement to be notified of when an interstitial has been dismissed for a given CBLocation.
 "Dismissal" is defined as any action that removed the interstitial UI such as a click or close.
 */
- (void)didDismissInterstitial:(CBLocation)location;

/*!
 @abstract
 Called after an interstitial has been closed.
 
 @param location The location for the Chartboost impression type.
 
 @discussion Implement to be notified of when an interstitial has been closed for a given CBLocation.
 "Closed" is defined as clicking the close interface for the interstitial.
 */
- (void)didCloseInterstitial:(CBLocation)location;

/*!
 @abstract
 Called after an interstitial has been clicked.
 
 @param location The location for the Chartboost impression type.
 
 @discussion Implement to be notified of when an interstitial has been click for a given CBLocation.
 "Clicked" is defined as clicking the creative interface for the interstitial.
 */
- (void)didClickInterstitial:(CBLocation)location;

/*!
 @abstract
 Called before an "more applications" will be displayed on the screen.
 
 @param location The location for the Chartboost impression type.
 
 @return YES if execution should proceed, NO if not.
 
 @discussion Implement to control if the Charboost SDK should display an "more applications"
 for the given CBLocation.  This is evaluated if the showMoreApps:(CBLocation)
 is called.  If YES is returned the operation will proceed, if NO, then the
 operation is treated as a no-op and nothing is displayed.
 
 Default return is YES.
 */
- (BOOL)shouldDisplayMoreApps:(CBLocation)location;

/*!
 @abstract
 Called after an "more applications" has been displayed on the screen.
 
 @param location The location for the Chartboost impression type.
 
 @discussion Implement to be notified of when an "more applications" has
 been displayed on the screen for a given CBLocation.
 */
- (void)didDisplayMoreApps:(CBLocation)location;

/*!
 @abstract
 Called after an "more applications" has been loaded from the Chartboost API
 servers and cached locally.
 
 @param location The location for the Chartboost impression type.
 
 @discussion Implement to be notified of when an "more applications" has been loaded from the Chartboost API
 servers and cached locally for a given CBLocation.
 */
- (void)didCacheMoreApps:(CBLocation)location;

/*!
 @abstract
 Called after an "more applications" has been dismissed.
 
 @param location The location for the Chartboost impression type.
 
 @discussion Implement to be notified of when an "more applications" has been dismissed for a given CBLocation.
 "Dismissal" is defined as any action that removed the "more applications" UI such as a click or close.
 */
- (void)didDismissMoreApps:(CBLocation)location;

/*!
 @abstract
 Called after an "more applications" has been closed.
 
 @param location The location for the Chartboost impression type.
 
 @discussion Implement to be notified of when an "more applications" has been closed for a given CBLocation.
 "Closed" is defined as clicking the close interface for the "more applications".
 */
- (void)didCloseMoreApps:(CBLocation)location;

/*!
 @abstract
 Called after an "more applications" has been clicked.
 
 @param location The location for the Chartboost impression type.
 
 @discussion Implement to be notified of when an "more applications" has been clicked for a given CBLocation.
 "Clicked" is defined as clicking the creative interface for the "more applications".
 */
- (void)didClickMoreApps:(CBLocation)location;

/*!
 @abstract
 Called after an "more applications" has attempted to load from the Chartboost API
 servers but failed.
 
 @param location The location for the Chartboost impression type.
 
 @param error The reason for the error defined via a CBLoadError.
 
 @discussion Implement to be notified of when an "more applications" has attempted to load from the Chartboost API
 servers but failed for a given CBLocation.
 */
- (void)didFailToLoadMoreApps:(CBLocation)location
                    withError:(CBLoadError)error;

#pragma mark - Rewarded Video Delegate

/*!
 @abstract
 Called before a rewarded video will be displayed on the screen.
 
 @param location The location for the Chartboost impression type.
 
 @return YES if execution should proceed, NO if not.
 
 @discussion Implement to control if the Charboost SDK should display a rewarded video
 for the given CBLocation.  This is evaluated if the showRewardedVideo:(CBLocation)
 is called.  If YES is returned the operation will proceed, if NO, then the
 operation is treated as a no-op and nothing is displayed.
 
 Default return is YES.
 */
- (BOOL)shouldDisplayRewardedVideo:(CBLocation)location;

/*!
 @abstract
 Called after a rewarded video has been displayed on the screen.
 
 @param location The location for the Chartboost impression type.
 
 @discussion Implement to be notified of when a rewarded video has
 been displayed on the screen for a given CBLocation.
 */
- (void)didDisplayRewardedVideo:(CBLocation)location;

/*!
 @abstract
 Called after a rewarded video has been loaded from the Chartboost API
 servers and cached locally.
 
 @param location The location for the Chartboost impression type.
 
 @discussion Implement to be notified of when a rewarded video has been loaded from the Chartboost API
 servers and cached locally for a given CBLocation.
 */
- (void)didCacheRewardedVideo:(CBLocation)location;

/*!
 @abstract
 Called after a rewarded video has attempted to load from the Chartboost API
 servers but failed.
 
 @param location The location for the Chartboost impression type.
 
 @param error The reason for the error defined via a CBLoadError.
 
 @discussion Implement to be notified of when an rewarded video has attempted to load from the Chartboost API
 servers but failed for a given CBLocation.
 */
- (void)didFailToLoadRewardedVideo:(CBLocation)location
                         withError:(CBLoadError)error;

/*!
 @abstract
 Called after a rewarded video has been dismissed.
 
 @param location The location for the Chartboost impression type.
 
 @discussion Implement to be notified of when a rewarded video has been dismissed for a given CBLocation.
 "Dismissal" is defined as any action that removed the rewarded video UI such as a click or close.
 */
- (void)didDismissRewardedVideo:(CBLocation)location;

/*!
 @abstract
 Called after a rewarded video has been closed.
 
 @param location The location for the Chartboost impression type.
 
 @discussion Implement to be notified of when a rewarded video has been closed for a given CBLocation.
 "Closed" is defined as clicking the close interface for the rewarded video.
 */
- (void)didCloseRewardedVideo:(CBLocation)location;

/*!
 @abstract
 Called after a rewarded video has been clicked.
 
 @param location The location for the Chartboost impression type.
 
 @discussion Implement to be notified of when a rewarded video has been click for a given CBLocation.
 "Clicked" is defined as clicking the creative interface for the rewarded video.
 */
- (void)didClickRewardedVideo:(CBLocation)location;

/*!
 @abstract
 Called after a rewarded video has been viewed completely and user is eligible for reward.
 
 @param reward The reward for watching the video.
 
 @param location The location for the Chartboost impression type.
 
 @discussion Implement to be notified of when a rewarded video has been viewed completely and user is eligible for reward.
 */
- (void)didCompleteRewardedVideo:(CBLocation)location
                      withReward:(int)reward;

#pragma mark - InPlay Delegate

/*!
 @abstract
 Called after an InPlay object has been loaded from the Chartboost API
 servers and cached locally.
 
 @param location The location for the Chartboost impression type.
 
 @discussion Implement to be notified of when an InPlay object has been loaded from the Chartboost API
 servers and cached locally for a given CBLocation.
 */
- (void)didCacheInPlay:(CBLocation)location;

/*!
 @abstract
 Called after a InPlay has attempted to load from the Chartboost API
 servers but failed.
 
 @param location The location for the Chartboost impression type.
 
 @param error The reason for the error defined via a CBLoadError.
 
 @discussion Implement to be notified of when an InPlay has attempted to load from the Chartboost API
 servers but failed for a given CBLocation.
 */
- (void)didFailToLoadInPlay:(CBLocation)location
                  withError:(CBLoadError)error;

#pragma mark - General Delegate

/*!
 @abstract
 Called before a video has been displayed on the screen.
 
 @param location The location for the Chartboost impression type.
 
 @discussion Implement to be notified of when a video will
 be displayed on the screen for a given CBLocation.  You can then do things like mute
 effects and sounds.
 */
- (void)willDisplayVideo:(CBLocation)location;

/*!
 @abstract
 Called after the App Store sheet is dismissed, when displaying the embedded app sheet.
 
 @discussion Implement to be notified of when the App Store sheet is dismissed.
 */
- (void)didCompleteAppStoreSheetFlow;

/*!
 @abstract
 Called if Chartboost SDK pauses click actions awaiting confirmation from the user.
 
 @discussion Use this method to display any gating you would like to prompt the user for input.
 Once confirmed call didPassAgeGate:(BOOL)pass to continue execution.
 */
- (void)didPauseClickForConfirmation;

#pragma mark - Deprecated Delegate

/*!
 @abstract
 Called before an "more applications" will be displayed on the screen.
 
 @return YES if execution should proceed, NO if not.
 
 @discussion Implement to control if the Charboost SDK should display an "more applications". 
 This is evaluated if the showMoreApps:(CBLocation) is called.  If YES is returned the operation will proceed, if NO, then the
 operation is treated as a no-op and nothing is displayed.
 
 Default return is YES.
 
 @deprecated This method has been deprecated and will be removed in a future version.
 */
- (BOOL)shouldDisplayMoreApps __attribute__((deprecated("As of version 4.5, use shouldDisplayMoreApps:(CBLocation)location")));;

/*!
 @abstract
 Called after an "more applications" has been displayed on the screen.
 
 @discussion Implement to be notified of when an "more applications" has
 been displayed on the screen.
 
 @deprecated This method has been deprecated and will be removed in a future version.
 */
- (void)didDisplayMoreApps __attribute__((deprecated("As of version 4.5, use didDisplayMoreApps:(CBLocation)location")));

/*!
 @abstract
 Called after an "more applications" has been loaded from the Chartboost API
 servers and cached locally.
 
 @discussion Implement to be notified of when an "more applications" has been loaded from the Chartboost API
 servers and cached locally.
 
 @deprecated This method has been deprecated and will be removed in a future version.
 */
- (void)didCacheMoreApps __attribute__((deprecated("As of version 4.5, use didCacheMoreApps:(CBLocation)location")));

/*!
 @abstract
 Called after an "more applications" has been dismissed.
 
 @discussion Implement to be notified of when an "more applications" has been dismissed.
 "Dismissal" is defined as any action that removed the "more applications" UI such as a click or close.
 
 @deprecated This method has been deprecated and will be removed in a future version.
 */
- (void)didDismissMoreApps __attribute__((deprecated("As of version 4.5, use didDismissMoreApps:(CBLocation)location")));

/*!
 @abstract
 Called after an "more applications" has been closed.
 
 @discussion Implement to be notified of when an "more applications" has been closed.
 "Closed" is defined as clicking the close interface for the "more applications".
 
 @deprecated This method has been deprecated and will be removed in a future version.
 */
- (void)didCloseMoreApps __attribute__((deprecated("As of version 4.5, use didCloseMoreApps:(CBLocation)location")));

/*!
 @abstract
 Called after an "more applications" has been clicked.
 
 @discussion Implement to be notified of when an "more applications" has been clicked.
 "Clicked" is defined as clicking the creative interface for the "more applications".
 
 @deprecated This method has been deprecated and will be removed in a future version.
 */
- (void)didClickMoreApps __attribute__((deprecated("As of version 4.5, use didClickMoreApps:(CBLocation)location")));

/*!
 @abstract
 Called after an "more applications" has attempted to load from the Chartboost API
 servers but failed.
 
 @param error The reason for the error defined via a CBLoadError.
 
 @discussion Implement to be notified of when an "more applications" has attempted to load from the Chartboost API
 servers but failed.
 
 @deprecated This method has been deprecated and will be removed in a future version.
 */
- (void)didFailToLoadMoreApps:(CBLoadError)error __attribute__((deprecated("As of version 4.5, use didFailToLoadMoreApps:(CBLoadError)error forLocation:(CBLocation)location")));

/*!
 @abstract
 Called after an InPlay object has been loaded from the Chartboost API
 servers and cached locally.
 
 @discussion Implement to be notified of when an InPlay object has been loaded from the Chartboost API
 servers and cached locally.
 
 @deprecated This method has been deprecated and will be removed in a future version.
 */
- (void)didLoadInPlay __attribute__((deprecated("As of version 4.5, use didCacheInPlay:(CBLocation)location")));

@end


