/*
 * CBAnalytics.h
 * Chartboost
 * 5.0.3
 *
 * Copyright 2011 Chartboost. All rights reserved.
 */

#import <StoreKit/StoreKit.h>

/*!
 @class ChartboostAnalytics
 
 @abstract
 Provide methods to track various events for improved targeting.
 
 @discussion For more information on integrating and using the Chartboost SDK
 please visit our help site documentation at https://help.chartboost.com
 */
@interface CBAnalytics : NSObject

/*!
 @abstract
 Track an In App Purchase Event.

 @param receipt The transaction receipt used to validate the purchase.
 
 @param productTitle The localized title of the product.
 
 @param productDescription The localized description of the product.
 
 @param productPrice The price of the product.
 
 @param productCurrency The localized currency of the product.
 
 @param productIdentifier The IOS identifier for the product.

 @discussion Tracks In App Purchases for later use with user segmentation
 and targeting.
*/
+ (void)trackInAppPurchaseEvent:(NSData *)receipt
                   productTitle:(NSString *)productTitle
             productDescription:(NSString *)productDescription
                   productPrice:(NSDecimalNumber *)productPrice
                productCurrency:(NSString *)productCurrency
              productIdentifier:(NSString *)productIdentifier;

/*!
 @abstract
 Track an In App Purchase Event.
 
 @param receipt The transaction receipt used to validate the purchase.
 
 @param product The SKProduct that was purchased.
 
 @discussion Tracks In App Purchases for later use with user segmentation
 and targeting.
 */
+ (void)trackInAppPurchaseEvent:(NSData *)receipt
                        product:(SKProduct *)product;

@end
