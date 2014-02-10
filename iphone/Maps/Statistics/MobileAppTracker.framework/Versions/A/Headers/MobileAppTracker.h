//
//  MobileAppTracker.h
//  MobileAppTracker
//
//  Created by HasOffers on 10/30/13.
//  Copyright (c) 2013 HasOffers. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MATEventItem.h"

#define MATVERSION @"3.0.1"


#pragma mark - enumerated types

/** @name Results for tracking actions */
typedef enum {
    MATActionResultFailure = -1,
    MATActionResultRequestSent = 1,
    MATActionResultAlreadyInstalled
} MATActionResult;

/** @name Gender type constants */
typedef enum {
    MATGenderMale=0,                   // Gender type MALE. Equals 0.
    MATGenderFemale=1,                 // Gender type FEMALE. Equals 1.
    
    MAT_GENDER_MALE=MATGenderMale,     // Backward-compatible alias for MATGenderMale.
    MAT_GENDER_FEMALE=MATGenderFemale  // Backward-compatible alias for MATGenderFemale.
} MATGender;


@protocol MobileAppTrackerDelegate;

/*!
 MobileAppTracker provides the methods to send events and actions to the
 HasOffers servers.
 */
@interface MobileAppTracker : NSObject


#pragma mark - Main Initializer

/** @name Intitializing MobileAppTracker With Advertiser Information */
/*!
 Starts Mobile App Tracker with MAT Advertiser Id and MAT Conversion Key. Both values are required.
 @param aid the MAT Advertiser Id provided in Mobile App Tracking.
 @param key the MAT Conversion Key provided in Mobile App Tracking.
 @return YES if error occurs, NO otherwise.
 */
+ (BOOL)startTrackerWithMATAdvertiserId:(NSString *)aid MATConversionKey:(NSString *)key;


#pragma mark - Delegate

/** @name MAT SDK Callback Delegate */
/*!
 [MobileAppTrackerDelegate](MobileAppTrackerDelegate) : A Delegate used by MobileAppTracker to post
 success and failure callbacks from the MAT SDK.
 */
+ (void)setDelegate:(id <MobileAppTrackerDelegate>)delegate;


#pragma mark - Debug And Test

/** @name Debug And Test */

/*!
 Specifies that the server responses should include debug information.
 
 @warning This is only for testing. You must turn this off for release builds.
 
 @param yesorno defaults to NO.
 */
+ (void)setDebugMode:(BOOL)yesorno;

/*!
 Set to YES to allow duplicate requests to be registered with the MAT server.
 
 @warning This is only for testing. You must turn this off for release builds.
 
 @param yesorno defaults to NO.
 */
+ (void)setAllowDuplicateRequests:(BOOL)yesorno;


#pragma mark - Data Parameters

/** @name Setter Methods */

/*!
 Set whether this is an existing user or a new one. This is generally used to
 distinguish users who were using previous versions of the app, prior to
 integration of the MAT SDK. The default is to assume a new user.
 See http://support.mobileapptracking.com/entries/22621001-Handling-Installs-prior-to-SDK-implementation
 @param existingUser - Is this a pre-existing user of the app? Default: NO
 */
+ (void)setExistingUser:(BOOL)existingUser;

/*!
 Set the Apple Advertising Identifier available in iOS 6.
 @param appleAdvertisingIdentifier - Apple Advertising Identifier
 */
+ (void)setAppleAdvertisingIdentifier:(NSUUID *)appleAdvertisingIdentifier;

/*!
 Set the Apple Vendor Identifier available in iOS 6.
 @param appleVendorIdentifier - Apple Vendor Identifier
 */
+ (void)setAppleVendorIdentifier:(NSUUID * )appleVendorIdentifier;

/*!
 Sets the currency code.
 Default: USD
 @param currencyCode The string name for the currency code.
 */
+ (void)setCurrencyCode:(NSString *)currencyCode;

/*!
 Sets the jailbroken device flag.
 @param yesorno The jailbroken device flag.
 */
+ (void)setJailbroken:(BOOL)yesorno;

/*!
 Sets the package name (bundle_id).
 Defaults to the Bundle Id of the app that is running the sdk.
 @param packageName The string name for the package.
 */
+ (void)setPackageName:(NSString *)packageName;

/*!
 Specifies if the sdk should auto detect if the iOS device is jailbroken.
 YES/NO
 @param yesorno YES will detect if the device is jailbroken, defaults to YES.
 */
+ (void)setShouldAutoDetectJailbroken:(BOOL)yesorno;

/*!
 Specifies if the sdk should pull the Apple Vendor Identifier from the device.
 YES/NO
 Note that setting to NO will clear any previously set value for the property.
 @param yesorno YES will set the Apple Vendor Identifier, defaults to NO.
 */
+ (void)setShouldAutoGenerateAppleVendorIdentifier:(BOOL)yesorno;

/*!
 Sets the site id.
 @param siteId The string id for the site id.
 */
+ (void)setSiteId:(NSString *)siteId;

/*!
 Set the TRUSTe Trusted Preference Identifier (TPID).
 @param tpid - Trusted Preference Identifier
 */
+ (void)setTRUSTeId:(NSString *)tpid;

/*!
 Sets the user id.
 @param userId The string name for the user id.
 */
+ (void)setUserId:(NSString *)userId;

/*!
 Set user's Facebook ID.
 @param facebookUserId string containing the user's Facebook user ID.
 */
+ (void)setFacebookUserId:(NSString *)facebookUserId;

/*!
 Set user's Twitter ID.
 @param twitterUserId string containing the user's Twitter user ID.
 */
+ (void)setTwitterUserId:(NSString *)twitterUserId;

/*!
 Set user's Google ID.
 @param googleUserId string containing the user's Google user ID.
 */
+ (void)setGoogleUserId:(NSString *)googleUserId;

/*!
 Sets the user's age.
 @param userAge user's age
 */
+ (void)setAge:(NSInteger)userAge;

/*!
 Sets the user's gender.
 @param userGender user's gender, possible values MATGenderMale, MATGenderFemale
 */
+ (void)setGender:(MATGender)userGender;

/*!
 Sets the user's location.
 @param latitude user's latitude
 @param longitude user's longitude
 */
+ (void)setLatitude:(double)latitude longitude:(double)longitude;

/*!
 Sets the user's location including altitude.
 @param latitude user's latitude
 @param longitude user's longitude
 @param altitude user's altitude
 */
+ (void)setLatitude:(double)latitude longitude:(double)longitude altitude:(double)altitude;

/*!
 Set app-level ad-tracking.
 YES/NO
 @param enable YES means opt-in, NO means opt-out.
 */
+ (void)setAppAdTracking:(BOOL)enable;

/*!
 Set the name of plugin used, if any. Not for general use.
 @param pluginName
 */
+ (void)setPluginName:(NSString *)pluginName;


/*!
 Set the first attribute to be included in the next track call.
 @param value
 */
+ (void)setEventAttribute1:(NSString*)value;

/*!
 Set the second attribute to be included in the next track call.
 @param value
 */
+ (void)setEventAttribute2:(NSString*)value;

/*!
 Set the third attribute to be included in the next track call.
 @param value
 */
+ (void)setEventAttribute3:(NSString*)value;

/*!
 Set the fourth attribute to be included in the next track call.
 @param value
 */
+ (void)setEventAttribute4:(NSString*)value;

/*!
 Set the fifth attribute to be included in the next track call.
 @param value
 */
+ (void)setEventAttribute5:(NSString*)value;


#pragma mark - Show iAd advertising

/** @name iAd advertising */

/*!
 Display an iAd banner in a parent view. The parent view will be faded in and out
 when an iAd is received or failed to display. The MobileAppTracker's delegate
 object will receive notifications when this happens.
 */
- (void) displayiAdInView:(UIView *)view;

/*!
 Removes the currently displayed iAd, if any.
 */
- (void) removeiAd;


#pragma mark - Track Install/Update Methods

/** @name Track Installs */

/*!
 To be called when an app opens; typically in the didFinishLaunching event.
 */
+ (MATActionResult)trackSession;

/*!
 To be called when an app opens; typically in the didFinishLaunching event.
 @param refId A reference id used to track an install and/or update, corresponds to advertiser_ref_id on the website.
 */
+ (MATActionResult)trackSessionWithReferenceId:(NSString *)refId;


#pragma mark - Track Actions

/** @name Track Actions */

/*!
 Record a Track Action for an Event Id or Name.
 @param eventIdOrName The event name or event id.
 */
+ (void)trackActionForEventIdOrName:(NSString *)eventIdOrName;

/*!
 Record a Track Action for an Event Id or Name and reference id.
 @param eventIdOrName The event name or event id.
 @param refId The referencId for an event, corresponds to advertiser_ref_id on the website.
 */
+ (void)trackActionForEventIdOrName:(NSString *)eventIdOrName
                        referenceId:(NSString *)refId;


/*!
 Record a Track Action for an Event Id or Name, revenue and currency.
 @param eventIdOrName The event name or event id.
 @param revenueAmount The revenue amount for the event.
 @param currencyCode The currency code override for the event. Blank defaults to sdk setting.
 */
+ (void)trackActionForEventIdOrName:(NSString *)eventIdOrName
                      revenueAmount:(float)revenueAmount
                       currencyCode:(NSString *)currencyCode;

/*!
 Record a Track Action for an Event Id or Name and reference id, revenue and currency.
 @param eventIdOrName The event name or event id.
 @param refId The referencId for an event, corresponds to advertiser_ref_id on the website.
 @param revenueAmount The revenue amount for the event.
 @param currencyCode The currency code override for the event. Blank defaults to sdk setting.
 */
+ (void)trackActionForEventIdOrName:(NSString *)eventIdOrName
                        referenceId:(NSString *)refId
                      revenueAmount:(float)revenueAmount
                       currencyCode:(NSString *)currencyCode;


#pragma mark - Track Actions With Event Items

/** @name Track Actions With Event Items */

/*!
 Record a Track Action for an Event Id or Name and a list of event items.
 @param eventIdOrName The event name or event id.
 @param eventItems An array of MATEventItem objects
 */
+ (void)trackActionForEventIdOrName:(NSString *)eventIdOrName
                         eventItems:(NSArray *)eventItems;

/*!
 Record a Track Action for an Event Name or Id.
 @param eventIdOrName The event name or event id.
 @param eventItems An array of MATEventItem objects
 @param refId The referencId for an event, corresponds to advertiser_ref_id on the website.
 */
+ (void)trackActionForEventIdOrName:(NSString *)eventIdOrName
                         eventItems:(NSArray *)eventItems
                        referenceId:(NSString *)refId;

/*!
 Record a Track Action for an Event Name or Id.
 @param eventIdOrName The event name or event id.
 @param eventItems An array of MATEventItem objects
 @param revenueAmount The revenue amount for the event.
 @param currencyCode The currency code override for the event. Blank defaults to sdk setting.
 */
+ (void)trackActionForEventIdOrName:(NSString *)eventIdOrName
                         eventItems:(NSArray *)eventItems
                      revenueAmount:(float)revenueAmount
                       currencyCode:(NSString *)currencyCode;

/*!
 Record a Track Action for an Event Name or Id.
 @param eventIdOrName The event name or event id.
 @param eventItems An array of MATEventItem objects
 @param refId The referencId for an event, corresponds to advertiser_ref_id on the website.
 @param revenueAmount The revenue amount for the event.
 @param currencyCode The currency code override for the event. Blank defaults to sdk setting.
 */
+ (void)trackActionForEventIdOrName:(NSString *)eventIdOrName
                         eventItems:(NSArray *)eventItems
                        referenceId:(NSString *)refId
                      revenueAmount:(float)revenueAmount
                       currencyCode:(NSString *)currencyCode;

/*!
 Record a Track Action for an Event Name or Id.
 @param eventIdOrName The event name or event id.
 @param eventItems An array of MATEventItem objects
 @param refId The referencId for an event, corresponds to advertiser_ref_id on the website.
 @param revenueAmount The revenue amount for the event.
 @param currencyCode The currency code override for the event. Blank defaults to sdk setting.
 @param transactionState The in-app purchase transaction SKPaymentTransactionState as received from the iTunes store.
 */
+ (void)trackActionForEventIdOrName:(NSString *)eventIdOrName
                         eventItems:(NSArray *)eventItems
                        referenceId:(NSString *)refId
                      revenueAmount:(float)revenueAmount
                       currencyCode:(NSString *)currencyCode
                   transactionState:(NSInteger)transactionState;

/*!
 Record a Track Action for an Event Name or Id.
 @param eventIdOrName The event name or event id.
 @param eventItems An array of MATEventItem objects
 @param refId The referencId for an event, corresponds to advertiser_ref_id on the website.
 @param revenueAmount The revenue amount for the event.
 @param currencyCode The currency code override for the event. Blank defaults to sdk setting.
 @param transactionState The in-app purchase transaction SKPaymentTransactionState as received from the iTunes store.
 @param receipt The in-app purchase transaction receipt as received from the iTunes store.
 */
+ (void)trackActionForEventIdOrName:(NSString *)eventIdOrName
                         eventItems:(NSArray *)eventItems
                        referenceId:(NSString *)refId
                      revenueAmount:(float)revenueAmount
                       currencyCode:(NSString *)currencyCode
                   transactionState:(NSInteger)transactionState
                            receipt:(NSData *)receipt;


#pragma mark - Cookie Tracking

/** @name Cookie Tracking */
/*!
 Sets whether or not to user cookie based tracking.
 Default: NO
 @param yesorno YES/NO for cookie based tracking.
 */
+ (void)setUseCookieTracking:(BOOL)yesorno;


#pragma mark - App-to-app Tracking

/** @name App-To-App Tracking */

/*!
 Sets a url to be used with app-to-app tracking so that
 the sdk can open the download (redirect) url. This is
 used in conjunction with the setTracking:advertiserId:offerId:publisherId:redirect: method.
 @param redirect_url The string name for the url.
 */
+ (void)setRedirectUrl:(NSString *)redirectURL;

/*!
 Start an app-to-app tracking session on the MAT server.
 @param targetAppPackageName The bundle identifier of the target app.
 @param targetAppAdvertiserId The MAT advertiser id of the target app.
 @param offerId The MAT offer id of the target app.
 @param publisherId The MAT publisher id of the target app.
 @param shouldRedirect Should redirect to the download url if the tracking session was 
   successfully created. See setRedirectUrl:.
 */
+ (void)setTracking:(NSString *)targetAppPackageName
       advertiserId:(NSString *)targetAppAdvertiserId
            offerId:(NSString *)targetAdvertiserOfferId
        publisherId:(NSString *)targetAdvertiserPublisherId
           redirect:(BOOL)shouldRedirect;


#pragma mark - Re-Engagement Method

/** @name Application Re-Engagement */

/*!
 Record the URL and Source when an application is opened via a URL scheme.
 This typically occurs during OAUTH or when an app exits and is returned
 to via a URL. The data will be sent to the HasOffers server so that a
 Re-Engagement can be recorded.
 @param urlString the url string used to open your app.
 @param sourceApplication the source used to open your app. For example, mobile safari.
 */
+ (void)applicationDidOpenURL:(NSString *)urlString sourceApplication:(NSString *)sourceApplication;

@end


#pragma mark - MobileAppTrackerDelegate

/** @name MobileAppTrackerDelegate */

/*!
 Protocol that allows for callbacks from the MobileAppTracker methods.
 To use, set your class as the delegate for MAT and implement these methods.
 */
@protocol MobileAppTrackerDelegate <NSObject>
@optional

/*!
 Delegate method called when a track action succeeds.
 @param tracker The MobileAppTracker instance.
 @param data The success data returned by the MobileAppTracker.
 */
- (void)mobileAppTrackerDidSucceedWithData:(NSData *)data;

/*!
 Delegate method called when a track action fails.
 @param tracker The MobileAppTracker instance.
 @param error Error object returned by the MobileAppTracker.
 */
- (void)mobileAppTrackerDidFailWithError:(NSError *)error;

/*!
 Delegate method called when an iAd is displayed and its parent view is faded in.
 */
- (void)mobileAppTrackerDidDisplayiAd;

/*!
 Delegate method called when an iAd failed to display and its parent view is faded out.
 */
- (void)mobileAppTrackerDidRemoveiAd;

/*!
 Delegate method called to pass through an iAd display error.
 @param error Error object returned by the iAd framework.
 */
- (void)mobileAppTrackerFailedToReceiveiAdWithError:(NSError *)error;

@end
