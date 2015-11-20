/**
 * Copyright (c) 2015-present, Parse, LLC.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#import <Foundation/Foundation.h>
#import <StoreKit/StoreKit.h>

#import <Parse/PFConstants.h>

PF_OSX_UNAVAILABLE_WARNING
PF_WATCH_UNAVAILABLE_WARNING

@class PFProduct;

NS_ASSUME_NONNULL_BEGIN

typedef void (^PFPurchaseProductObservationBlock)(SKPaymentTransaction *transaction);
typedef void (^PFPurchaseBuyProductResultBlock)(NSError *__nullable error);
typedef void (^PFPurchaseDownloadAssetResultBlock)(NSString *__nullable filePath, NSError *__nullable error);

/*!
 `PFPurchase` provides a set of APIs for working with in-app purchases.

 This class is currently for iOS only.
 */
PF_OSX_UNAVAILABLE PF_WATCH_UNAVAILABLE @interface PFPurchase : NSObject

/*!
 @abstract Add application logic block which is run when buying a product.

 @discussion This method should be called once for each product, and should be called before
 calling <buyProduct:block:>. All invocations to <addObserverForProduct:block:> should happen within
 the same method, and on the main thread. It is recommended to place all invocations of this method
 in `application:didFinishLaunchingWithOptions:`.

 @param productIdentifier the product identifier
 @param block The block to be run when buying a product.
 */
+ (void)addObserverForProduct:(NSString *)productIdentifier block:(PFPurchaseProductObservationBlock)block;

/*!
 @abstract *Asynchronously* initiates the purchase for the product.

 @param productIdentifier the product identifier
 @param block the completion block.
 */
+ (void)buyProduct:(NSString *)productIdentifier block:(nullable PFPurchaseBuyProductResultBlock)block;

/*!
 @abstract *Asynchronously* download the purchased asset, which is stored on Parse's server.

 @discussion Parse verifies the receipt with Apple and delivers the content only if the receipt is valid.

 @param transaction the transaction, which contains the receipt.
 @param completion the completion block.
 */
+ (void)downloadAssetForTransaction:(SKPaymentTransaction *)transaction
                         completion:(PFPurchaseDownloadAssetResultBlock)completion;

/*!
 @abstract *Asynchronously* download the purchased asset, which is stored on Parse's server.

 @discussion Parse verifies the receipt with Apple and delivers the content only if the receipt is valid.

 @param transaction the transaction, which contains the receipt.
 @param completion the completion block.
 @param progress the progress block, which is called multiple times to reveal progress of the download.
 */
+ (void)downloadAssetForTransaction:(SKPaymentTransaction *)transaction
                         completion:(PFPurchaseDownloadAssetResultBlock)completion
                           progress:(nullable PFProgressBlock)progress;

/*!
 @abstract *Asynchronously* restore completed transactions for the current user.

 @discussion Only nonconsumable purchases are restored. If observers for the products have been added before
 calling this method, invoking the method reruns the application logic associated with the purchase.

 @warning This method is only important to developers who want to preserve purchase states across
 different installations of the same app.
 */
+ (void)restore;

/*!
 @abstract Returns a content path of the asset of a product, if it was purchased and downloaded.

 @discussion To download and verify purchases use <downloadAssetForTransaction:completion:>.

 @warning This method will return `nil`, if the purchase wasn't verified or if the asset was not downloaded.
 */
+ (nullable NSString *)assetContentPathForProduct:(PFProduct *)product;

@end

NS_ASSUME_NONNULL_END
