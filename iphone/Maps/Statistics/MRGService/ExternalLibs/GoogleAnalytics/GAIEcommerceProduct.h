/*!
 @header    GAIEcommerceProduct.h
 @abstract  Google Analytics iOS SDK Hit Format Header
 @copyright Copyright 2014 Google Inc. All rights reserved.
 */

#import <Foundation/Foundation.h>

/*!
 * Class to construct product related information for a Google Analytics beacon. Use this class to
 * report information about products sold by merchants or impressions of products seen by users.
 * Instances of this class can be associated with both Product Actions and Product
 * Impression Lists.
 * <br>
 * Typical usage:
 * <code>
 * [tracker set:kGAIScreenName value:@"MyScreen"];
 * GAIDictionaryBuilder *builder = [GAIDictionaryBuilder createScreenView];
 * GAIEcommerceProduct *product = [[GAIEcommerceProduct alloc] init];
 * [product setId:@""PID-1234""];
 * [product setName:@"Space Monkeys!"];
 * [product setPrice:@100];
 * [product setQuantity:@2];
 * [builder addProductImpression:product impressionList:@"listName"];
 * [tracker send:[builder build]];
 * </code>
 */
@interface GAIEcommerceProduct : NSObject

/*!
 Sets the id that is used to identify a product in GA reports.
 */
- (GAIEcommerceProduct *)setId:(NSString *)productId;

/*!
 Sets the name that is used to indentify the product in GA reports.
 */
- (GAIEcommerceProduct *)setName:(NSString *)productName;

/*!
 Sets the brand associated with the product in GA reports.
 */
- (GAIEcommerceProduct *)setBrand:(NSString *)productBrand;

/*!
 Sets the category associated with the product in GA reports.
 */
- (GAIEcommerceProduct *)setCategory:(NSString *)productCategory;

/*!
 Sets the variant of the product.
 */
- (GAIEcommerceProduct *)setVariant:(NSString *)productVariant;

/*!
 Sets the price of the product.
 */
- (GAIEcommerceProduct *)setPrice:(NSNumber *)productPrice;

/*!
 Sets the quantity of the product.  This field is usually not used with product impressions.
 */
- (GAIEcommerceProduct *)setQuantity:(NSNumber *)productQuantity;

/*!
 Sets the coupon code associated with the product.  This field is usually not used with product
 impressions.
 */
- (GAIEcommerceProduct *)setCouponCode:(NSString *)productCouponCode;

/*!
 Sets the position of the product on the screen/product impression list, etc.
 */
- (GAIEcommerceProduct *)setPosition:(NSNumber *)productPosition;

/*!
 Sets the custom dimension associated with this product.
 */
- (GAIEcommerceProduct *)setCustomDimension:(NSUInteger)index value:(NSString *)value;

/*!
 Sets the custom metric associated with this product.
 */
- (GAIEcommerceProduct *)setCustomMetric:(NSUInteger)index value:(NSNumber *)value;

/*!
 Builds an NSDictionary of fields stored in this instance suitable for a product action.  The
 index parameter is the index of this product in the product action list.
 <br>
 Normally, users will have no need to call this method.
 */
- (NSDictionary *)buildWithIndex:(NSUInteger)index;

/*!
 Builds an NSDictionary of fields stored in this instance suitable for an impression list.  The
 lIndex parameter is the index of the product impression list while the index parameter is the
 index of this product in that impression list.
 <br>
 Normally, users will have no need to call this method.
 */
- (NSDictionary *)buildWithListIndex:(NSUInteger)lIndex index:(NSUInteger)index;
@end
