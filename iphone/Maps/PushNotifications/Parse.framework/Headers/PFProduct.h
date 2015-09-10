/**
 * Copyright (c) 2015-present, Parse, LLC.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#import <Foundation/Foundation.h>

#import <Parse/PFFile.h>
#import <Parse/PFNullability.h>
#import <Parse/PFObject.h>
#import <Parse/PFSubclassing.h>

PF_ASSUME_NONNULL_BEGIN

/*!
 The `PFProduct` class represents an in-app purchase product on the Parse server.
 By default, products can only be created via the Data Browser. Saving a `PFProduct` will result in error.
 However, the products' metadata information can be queried and viewed.

 This class is currently for iOS only.
 */
@interface PFProduct : PFObject<PFSubclassing>

///--------------------------------------
/// @name Product-specific Properties
///--------------------------------------

/*!
 @abstract The product identifier of the product.

 @discussion This should match the product identifier in iTunes Connect exactly.
 */
@property (PF_NULLABLE_PROPERTY nonatomic, strong) NSString *productIdentifier;

/*!
 @abstract The icon of the product.
 */
@property (PF_NULLABLE_PROPERTY nonatomic, strong) PFFile *icon;

/*!
 @abstract The title of the product.
 */
@property (PF_NULLABLE_PROPERTY nonatomic, strong) NSString *title;

/*!
 @abstract The subtitle of the product.
 */
@property (PF_NULLABLE_PROPERTY nonatomic, strong) NSString *subtitle;

/*!
 @abstract The order in which the product information is displayed in <PFProductTableViewController>.

 @discussion The product with a smaller order is displayed earlier in the <PFProductTableViewController>.
 */
@property (PF_NULLABLE_PROPERTY nonatomic, strong) NSNumber *order;

/*!
 @abstract The name of the associated download.

 @discussion If there is no downloadable asset, it should be `nil`.
 */
@property (PF_NULLABLE_PROPERTY nonatomic, strong, readonly) NSString *downloadName;

@end

PF_ASSUME_NONNULL_END
