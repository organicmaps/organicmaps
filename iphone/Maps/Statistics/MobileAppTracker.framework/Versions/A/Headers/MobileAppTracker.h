//
//  MobileAppTracker.h
//  MobileAppTracker
//
//  Created by HasOffers on 10/30/13.
//  Copyright (c) 2013 HasOffers. All rights reserved.
//

#import <Foundation/Foundation.h>

#define MATVERSION @"2.6.1"

@protocol MobileAppTrackerDelegate;

/*!
 MobileAppTracker provides the methods to send events and actions to the
 HasOffers servers.
 */
@interface MobileAppTracker : NSObject


#pragma mark - MATGender

/** @name Gender type constants */
/*!
 Gender type. An integer -- 0 or 1.
 */
typedef NSInteger MATGender;
/*!
 Gender type MALE. Equals 0.
 */
extern const NSInteger MAT_GENDER_MALE;
/*!
 Gender type FEMALE. Equals 1.
 */
extern const NSInteger MAT_GENDER_FEMALE;


#pragma mark - MobileAppTracker Shared Instance

/** @name MobileAppTracker Shared Instance */
/*!
 A singleton of the MobileAppTracker class
 */
+ (MobileAppTracker *)sharedManager;


#pragma mark - Main Initializer

/** @name Intitializing MobileAppTracker With Advertiser Information */
/*!
 Starts Mobile App Tracker with MAT Advertiser Id and MAT Conversion Key. Both values are required.
 @param aid the MAT Advertiser Id provided in Mobile App Tracking.
 @param key the MAT Conversion Key provided in Mobile App Tracking.
 @return YES if error occurs, NO otherwise.
 */
- (BOOL)startTrackerWithMATAdvertiserId:(NSString *)aid MATConversionKey:(NSString *)key;


#pragma mark - Properties

/** @name MAT SDK Callback Delegate */
/*!
 [MobileAppTrackerDelegate](MobileAppTrackerDelegate) : A Delegate used by MobileAppTracker to callback.
 Set this to receive success and failure callbacks from the MAT SDK.
 */
@property (nonatomic, assign) id <MobileAppTrackerDelegate> delegate;

/** @name MAT Data Parameters */
/*!
 Provides a view of the parameters used by the sdk at any time.
 */
@property (nonatomic, readonly) NSDictionary *sdkDataParameters;


#pragma mark - Debug And Test

/** @name Debug And Test */

/*!
 Specifies that the server responses should include debug information.

 @warning This is only for testing. You must turn this off for release builds.

 @param yesorno defaults to NO.
 */
- (void)setDebugMode:(BOOL)yesorno;

/*!
 Set to YES to allow duplicate requests to be registered with the MAT server.

 @warning This is only for testing. You must turn this off for release builds.

 @param yesorno defaults to NO.
 */
- (void)setAllowDuplicateRequests:(BOOL)yesorno;


#pragma mark - Data Parameters

/** @name Setter Methods */

/*!
 Set the Apple Advertising Identifier available in iOS 6.
 @param appleAdvertisingIdentifier - Apple Advertising Identifier
 */
- (void)setAppleAdvertisingIdentifier:(NSUUID *)appleAdvertisingIdentifier;

/*!
 Set the Apple Vendor Identifier available in iOS 6.
 @param appleVendorIdentifier - Apple Vendor Identifier
 */
- (void)setAppleVendorIdentifier:(NSUUID * )appleVendorIdentifier;

/*!
 Sets the currency code.
 Default: USD
 @param currencyCode The string name for the currency code.
 */
- (void)setCurrencyCode:(NSString *)currencyCode;

/*!
 Sets the jailbroken device flag.
 @param yesorno The jailbroken device flag.
 */
- (void)setJailbroken:(BOOL)yesorno;

/*!
 Sets the MAT advertiser id.
 @param advertiserId The string id for the MAT advertiser id.
 */
- (void)setMATAdvertiserId:(NSString *)advertiserId;

/*!
 Sets the MAT conversion key.
 @param conversionKey The string value for the MAT conversion key.
 */
- (void)setMATConversionKey:(NSString *)conversionKey;

/*!
 Sets the package name (bundle_id).
 Defaults to the Bundle Id of the app that is running the sdk.
 @param packageName The string name for the package.
 */
- (void)setPackageName:(NSString *)packageName;

/*!
 Specifies if the sdk should auto detect if the iOS device is jailbroken.
 YES/NO
 @param yesorno YES will detect if the device is jailbroken, defaults to YES.
 */
- (void)setShouldAutoDetectJailbroken:(BOOL)yesorno;

/*!
 Specifies if the sdk should pull the Apple Advertising Identifier from the device.
 YES/NO
 @param yesorno YES will set the Apple Advertising Identifier, defaults to NO.
 */
- (void)setShouldAutoGenerateAppleAdvertisingIdentifier:(BOOL)yesorno;

/*!
 Specifies if the sdk should pull the Apple Vendor Identifier from the device.
 YES/NO
 @param yesorno YES will set the Apple Vendor Identifier, defaults to NO.
 */
- (void)setShouldAutoGenerateAppleVendorIdentifier:(BOOL)yesorno;

/*!
 Sets the MAC address.
 @param macAddress mac address, defaults to nil.
 */
- (void)setMACAddress:(NSString *)macAddress;

/*!
 Sets the ODIN-1.
 @param odin1 ODIN-1, defaults to nil.
 */
- (void)setODIN1:(NSString *)odin1;

/*!
 Sets the OpenUDID.
 @param openUDID OpenUDID, defaults to nil.
 */
- (void)setOpenUDID:(NSString *)openUDID;

/*!
 Sets the site id.
 @param siteId The string id for the site id.
 */
- (void)setSiteId:(NSString *)siteId;

/*!
 Set the Trusted Preference Identifier (TPID).
 @param trusteTPID - Trusted Preference Identifier (TPID)
 */
- (void)setTrusteTPID:(NSString *)trusteTPID;

/*!
 Sets the user id.
 @param userId The string name for the user id.
 */
- (void)setUserId:(NSString *)userId;

/*!
 Sets the UIID. UIID is replacement of the deprecated UDID in iOS for Asia and Japan.
 Ref: https://github.com/akisute/UIApplication-UIID
 @param uiid UIID, defaults to nil.
 */
- (void)setUIID:(NSString *)uiid;

/*!
 Sets the user's age.
 @param userAge user's age
 */
- (void)setAge:(NSInteger)userAge;

/*!
 Sets the user's gender.
 @param userGender user's gender, possible values MAT_GENDER_MALE (0), MAT_GENDER_FEMALE (1).
 */
- (void)setGender:(MATGender)userGender;

/*!
 Sets the user's location.
 @param latitude user's latitude
 @param longitude user's longitude
 */
- (void)setLatitude:(double)latitude longitude:(double)longitude;

/*!
 Sets the user's location including altitude.
 @param latitude user's latitude
 @param longitude user's longitude
 @param altitude user's altitude
 */
- (void)setLatitude:(double)latitude longitude:(double)longitude altitude:(double)altitude;

/*!
 Set app-level ad-tracking.
 YES/NO
 @param enable YES means opt-in, NO means opt-out.
 */
- (void)setAppAdTracking:(BOOL)enable;


#pragma mark - Track Install/Update Methods

/** @name Track Installs and/or Updates */

/*!
 Record an Install or an Update by determining if a previous
 version of this app is already installed on the user's device.
 To be used if this is the first version of the app
 or the previous version also included MAT sdk.
 To be called when an app opens; typically in the didFinishLaunching event.
 Works only once per app version, does not have any effect if called again in the same app version.
 */
- (void)trackInstall;

/*!
 Record an Install or an Update by determining if a previous
 version of this app is already installed on the user's device.
 To be used if this is the first version of the app
 or the previous version also included MAT sdk.
 To be called when an app opens; typically in the didFinishLaunching event.
 Works only once per app version, does not have any effect if called again in the same app version.
 @param refId A reference id used to track an install and/or update, corresponds to advertiser_ref_id on the website.
 */
- (void)trackInstallWithReferenceId:(NSString *)refId;

/*!
 Instead of an Install, force an Update to be recorded.
 To be used if MAT sdk was not integrated in the previous
 version of this app. Only use this method if your app can distinguish
 between an install and an update, else use trackInstall.
 To be called when an app opens; typically in the didFinishLaunching event.
 Works only once per app version, does not have any effect if called again in the same app version.
 */
- (void)trackUpdate;

/*!
 Instead of an Install, force an Update to be recorded.
 To be used if MAT sdk was not integrated in the previous
 version of this app. Only use this method if your app can distinguish
 between an install and an update, else use trackInstallWithReferenceId.
 To be called when an app opens; typically in the didFinishLaunching event.
 Works only once per app version, does not have any effect if called again in the same app version.
 @param refId A reference id used to track an update, corresponds to advertiser_ref_id on the website.
 */
- (void)trackUpdateWithReferenceId:(NSString *)refId;


#pragma mark - Track Actions

/** @name Track Actions */

/*!
 Record a Track Action for an Event Id or Name.
 @param eventIdOrName The event name or event id.
 @param isId Yes if the event is an Id otherwise No if the event is a name only.
 */
- (void)trackActionForEventIdOrName:(NSString *)eventIdOrName
                          eventIsId:(BOOL)isId;

/*!
 Record a Track Action for an Event Id or Name and reference id.
 @param eventIdOrName The event name or event id.
 @param isId Yes if the event is an Id otherwise No if the event is a name only.
 @param refId The referencId for an event, corresponds to advertiser_ref_id on the website.
 */
- (void)trackActionForEventIdOrName:(NSString *)eventIdOrName
                          eventIsId:(BOOL)isId
                        referenceId:(NSString *)refId;


/*!
 Record a Track Action for an Event Id or Name, revenue and currency.
 @param eventIdOrName The event name or event id.
 @param isId Yes if the event is an Id otherwise No if the event is a name only.
 @param revenueAmount The revenue amount for the event.
 @param currencyCode The currency code override for the event. Blank defaults to sdk setting.
 */
- (void)trackActionForEventIdOrName:(NSString *)eventIdOrName
                          eventIsId:(BOOL)isId
                      revenueAmount:(float)revenueAmount
                       currencyCode:(NSString *)currencyCode;

/*!
 Record a Track Action for an Event Id or Name and reference id, revenue and currency.
 @param eventIdOrName The event name or event id.
 @param isId Yes if eventIdOrName is the event id for a pre-defined event on the MAT website, NO otherwise
 @param refId The referencId for an event, corresponds to advertiser_ref_id on the website.
 @param revenueAmount The revenue amount for the event.
 @param currencyCode The currency code override for the event. Blank defaults to sdk setting.
 */
- (void)trackActionForEventIdOrName:(NSString *)eventIdOrName
                          eventIsId:(BOOL)isId
                        referenceId:(NSString *)refId
                      revenueAmount:(float)revenueAmount
                       currencyCode:(NSString *)currencyCode;


#pragma mark - Track Actions With Event Items

/** @name Track Actions With Event Items */

/*!
 Record a Track Action for an Event Id or Name and a list of event items.
 @param eventIdOrName The event name or event id.
 @param isId Yes if the event is an Id otherwise No if the event is a name only.
 @param eventItems An array of MATEventItem objects
 */
- (void)trackActionForEventIdOrName:(NSString *)eventIdOrName
                          eventIsId:(BOOL)isId
                         eventItems:(NSArray *)eventItems;

/*!
 Record a Track Action for an Event Name or Id.
 @param eventIdOrName The event name or event id.
 @param isId Yes if the event is an Id otherwise No if the event is a name only.
 @param eventItems An array of MATEventItem objects
 @param refId The referencId for an event, corresponds to advertiser_ref_id on the website.
 */
- (void)trackActionForEventIdOrName:(NSString *)eventIdOrName
                          eventIsId:(BOOL)isId
                         eventItems:(NSArray *)eventItems
                        referenceId:(NSString *)refId;

/*!
 Record a Track Action for an Event Name or Id.
 @param eventIdOrName The event name or event id.
 @param isId Yes if the event is an Id otherwise No if the event is a name only.
 @param eventItems An array of MATEventItem objects
 @param revenueAmount The revenue amount for the event.
 @param currencyCode The currency code override for the event. Blank defaults to sdk setting.
 */
- (void)trackActionForEventIdOrName:(NSString *)eventIdOrName
                          eventIsId:(BOOL)isId
                         eventItems:(NSArray *)eventItems
                      revenueAmount:(float)revenueAmount
                       currencyCode:(NSString *)currencyCode;

/*!
 Record a Track Action for an Event Name or Id.
 @param eventIdOrName The event name or event id.
 @param isId Yes if the event is an Id otherwise No if the event is a name only.
 @param eventItems An array of MATEventItem objects
 @param refId The referencId for an event, corresponds to advertiser_ref_id on the website.
 @param revenueAmount The revenue amount for the event.
 @param currencyCode The currency code override for the event. Blank defaults to sdk setting.
 */
- (void)trackActionForEventIdOrName:(NSString *)eventIdOrName
                          eventIsId:(BOOL)isId
                         eventItems:(NSArray *)eventItems
                        referenceId:(NSString *)refId
                      revenueAmount:(float)revenueAmount
                       currencyCode:(NSString *)currencyCode;

/*!
 Record a Track Action for an Event Name or Id.
 @param eventIdOrName The event name or event id.
 @param isId Yes if the event is an Id otherwise No if the event is a name only.
 @param eventItems An array of MATEventItem objects
 @param refId The referencId for an event, corresponds to advertiser_ref_id on the website.
 @param revenueAmount The revenue amount for the event.
 @param currencyCode The currency code override for the event. Blank defaults to sdk setting.
 @param transactionState The in-app purchase transaction SKPaymentTransactionState as received from the iTunes store.
 */
- (void)trackActionForEventIdOrName:(NSString *)eventIdOrName
                          eventIsId:(BOOL)isId
                         eventItems:(NSArray *)eventItems
                        referenceId:(NSString *)refId
                      revenueAmount:(float)revenueAmount
                       currencyCode:(NSString *)currencyCode
                   transactionState:(NSInteger)transactionState;

/*!
 Record a Track Action for an Event Name or Id.
 @param eventIdOrName The event name or event id.
 @param isId Yes if the event is an Id otherwise No if the event is a name only.
 @param eventItems An array of MATEventItem objects
 @param refId The referencId for an event, corresponds to advertiser_ref_id on the website.
 @param revenueAmount The revenue amount for the event.
 @param currencyCode The currency code override for the event. Blank defaults to sdk setting.
 @param transactionState The in-app purchase transaction SKPaymentTransactionState as received from the iTunes store.
 @param receipt The in-app purchase transaction receipt as received from the iTunes store.
 */
- (void)trackActionForEventIdOrName:(NSString *)eventIdOrName
                          eventIsId:(BOOL)isId
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
- (void)setUseCookieTracking:(BOOL)yesorno;


#pragma mark - App-to-app Tracking

/** @name App-To-App Tracking */

/*!
 Sets a url to be used with app-to-app tracking so that
 the sdk can open the download (redirect) url. This is
 used in conjunction with the setTracking:advertiserId:offerId:publisherId:redirect: method.
 @param redirect_url The string name for the url.
 */
- (void)setRedirectUrl:(NSString *)redirectURL;

/*!
 Start a Tracking Session on the MAT server.
 @param targetAppPackageName The bundle identifier of the target app.
 @param targetAppAdvertiserId The MAT advertiser id of the target app.
 @param offerId The MAT offer id of the target app.
 @param publisherId The MAT publisher id of the target app.
 @param shouldRedirect Should redirect to the download url if the tracking session was successfully created. See setRedirectUrl:.
 */
- (void)setTracking:(NSString *)targetAppPackageName
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
- (void)applicationDidOpenURL:(NSString *)urlString sourceApplication:(NSString *)sourceApplication;

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
- (void)mobileAppTracker:(MobileAppTracker *)tracker didSucceedWithData:(NSData *)data;

/*!
 Delegate method called when a track action fails.
 @param tracker The MobileAppTracker instance.
 @param error Error object returned by the MobileAppTracker.
 */
- (void)mobileAppTracker:(MobileAppTracker *)tracker didFailWithError:(NSError *)error;

@end


#pragma mark - MATEventItem

/*!
 MATEventItem represents event items for use with MAT events.
 */
@interface MATEventItem : NSObject

/** @name MATEventItem Properties */
/*!
 name of the event item
 */
@property (nonatomic, copy) NSString *item;
/*!
 unit price of the event item
 */
@property (nonatomic, assign) float unitPrice;
/*!
 quantity of the event item
 */
@property (nonatomic, assign) int quantity;
/*!
 revenue of the event item
 */
@property (nonatomic, assign) float revenue;

/*!
 an extra parameter that corresponds to attribute_sub1 property of the event item
 */
@property (nonatomic, copy) NSString *attribute1;
/*!
 an extra parameter that corresponds to attribute_sub2 property of the event item
 */
@property (nonatomic, copy) NSString *attribute2;
/*!
 an extra parameter that corresponds to attribute_sub3 property of the event item
 */
@property (nonatomic, copy) NSString *attribute3;
/*!
 an extra parameter that corresponds to attribute_sub4 property of the event item
 */
@property (nonatomic, copy) NSString *attribute4;
/*!
 an extra parameter that corresponds to attribute_sub5 property of the event item
 */
@property (nonatomic, copy) NSString *attribute5;


/** @name Methods to create MATEventItem objects.*/

/*!
 Method to create an event item. Revenue will be calculated using (quantity * unitPrice).

 @param name name of the event item
 @param unitPrice unit price of the event item
 @param quantity quantity of the event item
 */
+ (MATEventItem *)eventItemWithName:(NSString *)name unitPrice:(float)unitPrice quantity:(int)quantity;

/*!
 Method to create an event item.
 @param name name of the event item
 @param unitPrice unit price of the event item
 @param quantity quantity of the event item
 @param revenue revenue of the event item, to be used instead of (quantity * unitPrice)
 */
+ (MATEventItem *)eventItemWithName:(NSString *)name unitPrice:(float)unitPrice quantity:(int)quantity revenue:(float)revenue;

/*!
 Method to create an event item.
 @param name name of the event item
 @param attribute1 an extra parameter that corresponds to attribute_sub1 property of the event item
 @param attribute2 an extra parameter that corresponds to attribute_sub2 property of the event item
 @param attribute3 an extra parameter that corresponds to attribute_sub3 property of the event item
 @param attribute4 an extra parameter that corresponds to attribute_sub4 property of the event item
 @param attribute5 an extra parameter that corresponds to attribute_sub5 property of the event item
 */
+ (MATEventItem *)eventItemWithName:(NSString *)name
                         attribute1:(NSString *)attribute1
                         attribute2:(NSString *)attribute2
                         attribute3:(NSString *)attribute3
                         attribute4:(NSString *)attribute4
                         attribute5:(NSString *)attribute5;

/*!
 Method to create an event item.
 @param name name of the event item
 @param unitPrice unit price of the event item
 @param quantity quantity of the event item
 @param revenue revenue of the event item, to be used instead of (quantity * unitPrice)
 @param attribute1 an extra parameter that corresponds to attribute_sub1 property of the event item
 @param attribute2 an extra parameter that corresponds to attribute_sub2 property of the event item
 @param attribute3 an extra parameter that corresponds to attribute_sub3 property of the event item
 @param attribute4 an extra parameter that corresponds to attribute_sub4 property of the event item
 @param attribute5 an extra parameter that corresponds to attribute_sub5 property of the event item
 */
+ (MATEventItem *)eventItemWithName:(NSString *)name unitPrice:(float)unitPrice quantity:(int)quantity revenue:(float)revenue
                         attribute1:(NSString *)attribute1
                         attribute2:(NSString *)attribute2
                         attribute3:(NSString *)attribute3
                         attribute4:(NSString *)attribute4
                         attribute5:(NSString *)attribute5;
@end
