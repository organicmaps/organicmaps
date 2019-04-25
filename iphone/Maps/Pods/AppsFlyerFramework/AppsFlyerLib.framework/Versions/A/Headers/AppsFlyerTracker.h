//
//  AppsFlyerTracker.h
//  AppsFlyerLib
//
//  AppsFlyer iOS SDK 4.9.0 (813)
//  Copyright (c) 2019 AppsFlyer Ltd. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "AppsFlyerCrossPromotionHelper.h"
#import "AppsFlyerShareInviteHelper.h"



// In app event names constants
#define AFEventLevelAchieved            @"af_level_achieved"
#define AFEventAddPaymentInfo           @"af_add_payment_info"
#define AFEventAddToCart                @"af_add_to_cart"
#define AFEventAddToWishlist            @"af_add_to_wishlist"
#define AFEventCompleteRegistration     @"af_complete_registration"
#define AFEventTutorial_completion      @"af_tutorial_completion"
#define AFEventInitiatedCheckout        @"af_initiated_checkout"
#define AFEventPurchase                 @"af_purchase"
#define AFEventRate                     @"af_rate"
#define AFEventSearch                   @"af_search"
#define AFEventSpentCredits             @"af_spent_credits"
#define AFEventAchievementUnlocked      @"af_achievement_unlocked"
#define AFEventContentView              @"af_content_view"
#define AFEventListView                 @"af_list_view"
#define AFEventTravelBooking            @"af_travel_booking"
#define AFEventShare                    @"af_share"
#define AFEventInvite                   @"af_invite"
#define AFEventLogin                    @"af_login"
#define AFEventReEngage                 @"af_re_engage"
#define AFEventUpdate                   @"af_update"
#define AFEventOpenedFromPushNotification @"af_opened_from_push_notification"
#define AFEventLocation                 @"af_location_coordinates"
#define AFEventCustomerSegment          @"af_customer_segment"

#define AFEventSubscribe                @"af_subscribe"
#define AFEventStartTrial               @"af_start_trial"
#define AFEventAdClick                  @"af_ad_click"
#define AFEventAdView                   @"af_ad_view"

// In app event parameter names
#define AFEventParamContent                @"af_content"
#define AFEventParamAchievenmentId         @"af_achievement_id"
#define AFEventParamLevel                  @"af_level"
#define AFEventParamScore                  @"af_score"
#define AFEventParamSuccess                @"af_success"
#define AFEventParamPrice                  @"af_price"
#define AFEventParamContentType            @"af_content_type"
#define AFEventParamContentId              @"af_content_id"
#define AFEventParamContentList            @"af_content_list"
#define AFEventParamCurrency               @"af_currency"
#define AFEventParamQuantity               @"af_quantity"
#define AFEventParamRegistrationMethod     @"af_registration_method"
#define AFEventParamPaymentInfoAvailable   @"af_payment_info_available"
#define AFEventParamMaxRatingValue         @"af_max_rating_value"
#define AFEventParamRatingValue            @"af_rating_value"
#define AFEventParamSearchString           @"af_search_string"
#define AFEventParamDateA                  @"af_date_a"
#define AFEventParamDateB                  @"af_date_b"
#define AFEventParamDestinationA           @"af_destination_a"
#define AFEventParamDestinationB           @"af_destination_b"
#define AFEventParamDescription            @"af_description"
#define AFEventParamClass                  @"af_class"
#define AFEventParamEventStart             @"af_event_start"
#define AFEventParamEventEnd               @"af_event_end"
#define AFEventParamLat                    @"af_lat"
#define AFEventParamLong                   @"af_long"
#define AFEventParamCustomerUserId         @"af_customer_user_id"
#define AFEventParamValidated              @"af_validated"
#define AFEventParamRevenue                @"af_revenue"
#define AFEventProjectedParamRevenue       @"af_projected_revenue"
#define AFEventParamReceiptId              @"af_receipt_id"
#define AFEventParamTutorialId             @"af_tutorial_id"
#define AFEventParamAchievenmentId         @"af_achievement_id"
#define AFEventParamVirtualCurrencyName    @"af_virtual_currency_name"
#define AFEventParamDeepLink               @"af_deep_link"
#define AFEventParamOldVersion             @"af_old_version"
#define AFEventParamNewVersion             @"af_new_version"
#define AFEventParamReviewText             @"af_review_text"
#define AFEventParamCouponCode             @"af_coupon_code"
#define AFEventParamOrderId                @"af_order_id"
#define AFEventParam1                      @"af_param_1"
#define AFEventParam2                      @"af_param_2"
#define AFEventParam3                      @"af_param_3"
#define AFEventParam4                      @"af_param_4"
#define AFEventParam5                      @"af_param_5"
#define AFEventParam6                      @"af_param_6"
#define AFEventParam7                      @"af_param_7"
#define AFEventParam8                      @"af_param_8"
#define AFEventParam9                      @"af_param_9"
#define AFEventParam10                     @"af_param_10"

#define AFEventParamDepartingDepartureDate  @"af_departing_departure_date"
#define AFEventParamReturningDepartureDate  @"af_returning_departure_date"
#define AFEventParamDestinationList         @"af_destination_list"  //array of string
#define AFEventParamCity                    @"af_city"
#define AFEventParamRegion                  @"af_region"
#define AFEventParamCountry                 @"af_country"


#define AFEventParamDepartingArrivalDate    @"af_departing_arrival_date"
#define AFEventParamReturningArrivalDate    @"af_returning_arrival_date"
#define AFEventParamSuggestedDestinations   @"af_suggested_destinations" //array of string
#define AFEventParamTravelStart             @"af_travel_start"
#define AFEventParamTravelEnd               @"af_travel_end"
#define AFEventParamNumAdults               @"af_num_adults"
#define AFEventParamNumChildren             @"af_num_children"
#define AFEventParamNumInfants              @"af_num_infants"
#define AFEventParamSuggestedHotels         @"af_suggested_hotels" //array of string

#define AFEventParamUserScore               @"af_user_score"
#define AFEventParamHotelScore              @"af_hotel_score"
#define AFEventParamPurchaseCurrency        @"af_purchase_currency"

#define AFEventParamPreferredStarRatings    @"af_preferred_star_ratings"    //array of int (basically a tuple (min,max) but we'll use array of int and instruct the developer to use two values)

#define AFEventParamPreferredPriceRange     @"af_preferred_price_range"    //array of int (basically a tuple (min,max) but we'll use array of int and instruct the developer to use two values)
#define AFEventParamPreferredNeighborhoods  @"af_preferred_neighborhoods" //array of string
#define AFEventParamPreferredNumStops       @"af_preferred_num_stops"

#define AFEventParamAdRevenueAdType              @"af_adrev_ad_type"
#define AFEventParamAdRevenueNetworkName         @"af_adrev_network_name"
#define AFEventParamAdRevenuePlacementId         @"af_adrev_placement_id"
#define AFEventParamAdRevenueAdSize              @"af_adrev_ad_size"
#define AFEventParamAdRevenueMediatedNetworkName @"af_adrev_mediated_network_name"

#define kDefaultOneLink          @"go.onelink.me"
#define kNoOneLinkFallback       @"https://app.appsflyer.com"
#define kINviteAppleAppID        @"af_siteid"




/// Mail hashing type
typedef enum  {
    /// None
    EmailCryptTypeNone = 0,
    /// SHA1
    EmailCryptTypeSHA1 = 1,
    /// MD5
    EmailCryptTypeMD5 = 2,
    /// SHA256
    EmailCryptTypeSHA256 = 3
} EmailCryptType;

/**
 Conform and subscribe to this protocol to allow getting data about conversion and
 install attribution
 */
@protocol AppsFlyerTrackerDelegate <NSObject>

@optional
/**
 `installData` contains information about install.
 Organic/non-organic, etc.
 */
- (void)onConversionDataReceived:(NSDictionary *)installData;

/**
 Any errors that occurred during the conversion request.
 */
- (void)onConversionDataRequestFailure:(NSError *)error;

/**
 `attributionData` contains information about OneLink, deeplink.
 */
- (void)onAppOpenAttribution:(NSDictionary *)attributionData;

/**
 Any errors that occurred during the attribution request.
 */
- (void)onAppOpenAttributionFailure:(NSError *)error;

@end

/**
 You can track installs, app updates, sessions and additional in-app events
 (including in-app purchases, game levels, etc.)
 to evaluate ROI and user engagement.
 The iOS SDK is compatible with all iOS/tvOS devices with iOS version 7 and above.
 
 @see [SDK Integration Validator](https://support.appsflyer.com/hc/en-us/articles/207032066-AppsFlyer-SDK-Integration-iOS)
 for more information.
 
 */
@interface AppsFlyerTracker : NSObject

/**
 Gets the singleton instance of the AppsFlyerTracker class, creating it if
 necessary.
 
 @return The singleton instance of AppsFlyerTracker.
 */
+ (AppsFlyerTracker *)sharedTracker;

/**
 In case you use your own user ID in your app, you can set this property to that ID.
 Enables you to cross-reference your own unique ID with AppsFlyer’s unique ID and the other devices’ IDs
 */
@property(nonatomic, strong, setter = setCustomerUserID:) NSString * customerUserID;

/**
 In case you use custom data and you want to receive it in the raw reports.
 
 @see [Setting additional custom data](https://support.appsflyer.com/hc/en-us/articles/207032066-AppsFlyer-SDK-Integration-iOS#setting-additional-custom-data) for more information.
 */
@property(nonatomic, strong, setter = setAdditionalData:) NSDictionary * customData;

/**
 Use this property to set your AppsFlyer's dev key
 */
@property(nonatomic, strong, setter = setAppsFlyerDevKey:) NSString * appsFlyerDevKey;

/**
 Use this property to set your app's Apple ID(taken from the app's page on iTunes Connect)
 */
@property(nonatomic, strong, setter = setAppleAppID:) NSString * appleAppID;

/**
 In case of in app purchase events, you can set the currency code your user has purchased with.
 The currency code is a 3 letter code according to ISO standards
 
 Objective-C:
 
 <pre>
 [[AppsFlyerTracker sharedTracker] setCurrencyCode:@"USD"];
 </pre>
 
 Swift:
 
 <pre>
 AppsFlyerTracker.shared().currencyCode = "USD"
 </pre>
 */
@property(nonatomic, strong) NSString *currencyCode;

/**
 AppsFlyer SDK collect Apple's `advertisingIdentifier` if the `AdSupport.framework` included in the SDK.
 You can disable this behavior by setting the following property to YES
 */
@property(atomic) BOOL disableAppleAdSupportTracking;

/**
 Prints SDK messages to the console log. This property should only be used in `DEBUG` mode.
 The default value is `NO`
 */
@property(nonatomic, setter = setIsDebug:) BOOL isDebug;

/**
 Set this flag to `YES`, to collect the current device name(e.g. "My iPhone"). Default value is `NO`
 */
@property(nonatomic, setter = setShouldCollectDeviceName:) BOOL shouldCollectDeviceName;

/**
 Set your `OneLink ID` from OneLink configuration. Used in User Invites to generate a OneLink.
 */
@property(nonatomic, strong, setter = setAppInviteOneLink:) NSString * appInviteOneLinkID;

/**
 Opt-out tracking for specific user
 */
@property(atomic) BOOL deviceTrackingDisabled;

/**
 Opt-out tracking for Apple Search Ads attributions
 */
@property(atomic) BOOL disableIAdTracking;

/**
 AppsFlyer delegate. See `AppsFlyerTrackerDelegate`
 */
@property(weak, nonatomic) id<AppsFlyerTrackerDelegate> delegate;

/**
 In app purchase receipt validation Apple environment(production or sandbox). The default value is NO
 */
@property(nonatomic, setter = setUseReceiptValidationSandbox:) BOOL useReceiptValidationSandbox;

/**
 Set this flag to test uninstall on Apple environment(production or sandbox). The default value is NO
 */
@property(nonatomic, setter = setUseUninstallSandbox:) BOOL useUninstallSandbox;

/**
 Advertising Id(exposed for RemoteDebug)
 */
@property(nonatomic, strong) NSString *advertiserId;

/**
 For advertisers who wrap OneLink within another Universal Link.
 An advertiser will be able to deeplink from a OneLink wrapped within another Universal Link and also track this retargeting conversion.
 
 Objective-C:
 
 <pre>
 [[AppsFlyerTracker sharedTracker] setResolveDeepLinkURLs:@[@"domain.com", @"subdomain.domain.com"]];
 </pre>
 */
@property(nonatomic) NSArray<NSString *> *resolveDeepLinkURLs;

/**
 Use this to send the user's emails
 
 @param userEmails The list of strings that hold mails
 @param type Hash algoritm
 */
- (void)setUserEmails:(NSArray *)userEmails withCryptType:(EmailCryptType)type;

/**
 Track application launch(session).
 Add the following method at the `applicationDidBecomeActive` in AppDelegate class
 */
- (void)trackAppLaunch;

/**
 Use this method to track events in your app like purchases or user actions
 
 @param eventName Contains name of event that could be provided from predefined constants in `AppsFlyerTracker.h`
 @param value Contains value for handling by backend
 
 <pre>
 [[AppsFlyer sharedTracker] trackEvent:AFEventPurchase withValue:"200"];
 </pre>
 
 */
- (void)trackEvent:(NSString *)eventName withValue:(NSString *)value __attribute__((deprecated));

/**
 Use this method to track an events with mulitple values. See AppsFlyer's documentation for details.
 
 Objective-C:
 
 <pre>
 [[AppsFlyerTracker sharedTracker] trackEvent:AFEventPurchase
        withValues: @{AFEventParamRevenue  : @200,
                      AFEventParamCurrency : @"USD",
                      AFEventParamQuantity : @2,
                      AFEventParamContentId: @"092",
                      AFEventParamReceiptId: @"9277"}];
 </pre>
 
 Swift:
 
 <pre>
 AppsFlyerTracker.shared().trackEvent(AFEventPurchase,
        withValues: [AFEventParamRevenue  : "1200",
                     AFEventParamContent  : "shoes",
                     AFEventParamContentId: "123"])
 </pre>
 
 @param eventName Contains name of event that could be provided from predefined constants in `AppsFlyerTracker.h`
 @param values Contains dictionary of values for handling by backend
 */
- (void)trackEvent:(NSString *)eventName withValues:(NSDictionary *)values;

/**
 To track and validate in app purchases you can call this method from the completeTransaction: method on
 your `SKPaymentTransactionObserver`.
 
 @param productIdentifier The product identifier
 @param price The product price
 @param currency The product currency
 @param tranactionId The purchase transaction Id
 @param params The additional param, which you want to receive it in the raw reports
 @param successBlock The success callback
 @param failedBlock The failure callback
 */
- (void)validateAndTrackInAppPurchase:(NSString *)productIdentifier
                                 price:(NSString *)price
                              currency:(NSString *)currency
                         transactionId:(NSString *)tranactionId
                  additionalParameters:(NSDictionary *)params
                               success:(void (^)(NSDictionary *response))successBlock
                               failure:(void (^)(NSError *error, id reponse))failedBlock NS_AVAILABLE(10_7, 7_0);

/**
 To Track location for geo-fencing. Does the same as code below.
 
 <pre>
 AppsFlyerTracker.shared().trackEvent(AFEventLocation, withValues: [AFEventParamLong:longitude, AFEventParamLat:latitude])
 </pre>
 
 @param longitude The location longitude
 @param latitude The location latitude
 */
- (void)trackLocation:(double)longitude latitude:(double)latitude;

/**
 This method returns AppsFlyer's internal id(unique for your app)
 
 @return Internal AppsFlyer Id
 */
- (NSString *)getAppsFlyerUID;

/**
 In case you want to use AppsFlyer tracking data in your app you can use the following method set a
 delegate with callback buttons for the tracking data. See AppsFlyerTrackerDelegate above.
 
 @param delegate The AppsFlyer delegate reference
 */
- (void)loadConversionDataWithDelegate:(id<AppsFlyerTrackerDelegate>)delegate __attribute__((deprecated));

/**
 In case you want to track deep linking. Does the same as `-handleOpenURL:sourceApplication:withAnnotation`.
 
 @warning Prefered to use `-handleOpenURL:sourceApplication:withAnnotation`.
 
 @param url The URL that was passed to your AppDelegate.
 @param sourceApplication The sourceApplication that passed to your AppDelegate.
 */
- (void)handleOpenURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication;

/**
 In case you want to track deep linking.
 Call this method from inside your AppDelegate `-application:openURL:sourceApplication:annotation:`
 
 @param url The URL that was passed to your AppDelegate.
 @param sourceApplication The sourceApplication that passed to your AppDelegate.
 @param annotation The annotation that passed to your app delegate.
 */
- (void)handleOpenURL:(NSURL *)url
    sourceApplication:(NSString *)sourceApplication
       withAnnotation:(id)annotation;

/**
 Call this method from inside of your AppDelegate `-application:openURL:options:` method.
 This method is functionally the same as calling the AppsFlyer method
 `-handleOpenURL:sourceApplication:withAnnotation`.
 
 @param url The URL that was passed to your app delegate
 @param options The options dictionary that was passed to your AppDelegate.
 */
- (void)handleOpenUrl:(NSURL *)url options:(NSDictionary *)options;

/**
 Allow AppsFlyer to handle restoration from an NSUserActivity.
 Use this method to track deep links with OneLink.
 
 @param userActivity The NSUserActivity that caused the app to be opened.
 */
- (BOOL)continueUserActivity:(NSUserActivity *)userActivity
          restorationHandler:(void (^)(NSArray *))restorationHandler NS_AVAILABLE_IOS(9_0);

/**
 This method is not used anymore. Exist only for backward compatability. Don't use.
 
 @param userActivity The NSUserActivity param.
 */
- (void)didUpdateUserActivity:(NSUserActivity *)userActivity NS_AVAILABLE_IOS(9_0);

/**
 Enable AppsFlyer to handle a push notification.
 
 @see [Learn more here](https://support.appsflyer.com/hc/en-us/articles/207364076-Measuring-Push-Notification-Re-Engagement-Campaigns)
 
 @warning To make it work - set data, related to AppsFlyer under key @"af".
 
 @param pushPayload The `userInfo` from received remote notification. One of root keys should be @"af".
 */
- (void)handlePushNotification:(NSDictionary *)pushPayload;


/**
 Register uninstall - you should register for remote notification and provide AppsFlyer the push device token.
 
 @param deviceToken The `deviceToken` from `-application:didRegisterForRemoteNotificationsWithDeviceToken:`
 */
- (void)registerUninstall:(NSData *)deviceToken;

/**
 Get SDK version.
 
 @return The AppsFlyer SDK version info.
 */
- (NSString *)getSDKVersion;

/**
 This is for internal use.
 */
- (void)remoteDebuggingCallWithData:(NSString *)data;

/**
 @brief This property accepts a string value representing the host name for all endpoints.
 Can be used to Zero rate your application’s data usage. Contact your CSM for more information.
 
 @warning To use `default` SDK endpoint – set value to `nil`.
 
 Objective-C:
 
 <pre>
 [[AppsFlyerTracker sharedTracker] setHost:@"example.com"];
 </pre>
 
 Swift:
 
 <pre>
 AppsFlyerTracker.shared().host = "example.com"
 </pre>
 */
@property(nonatomic, strong) NSString *host;

- (void)setHost:(NSString *)host DEPRECATED_MSG_ATTRIBUTE("Use -[AppsFlyerTracker setHost:withHostPrefix:] instead");

/**
 * This function set the host name and prefix host name for all the endpoints
 **/
- (void)setHost:(NSString *)host withHostPrefix:(NSString *)hostPrefix;

/**
 * This property accepts a string value representing the prefix host name for all endpoints.
 * for example "test" prefix with default host name will have the address "host.appsflyer.com"
 */
@property(nonatomic, strong, readonly) NSString *hostPrefix;

/**
 This property is responsible for timeout between sessions in seconds.
 Default value is 5 seconds.
 */
@property(atomic) NSUInteger minTimeBetweenSessions;

/**
 API to shut down all SDK activities.
 
 @warning This will disable all requests from AppsFlyer SDK.
 */
@property(atomic) BOOL isStopTracking;

@end
