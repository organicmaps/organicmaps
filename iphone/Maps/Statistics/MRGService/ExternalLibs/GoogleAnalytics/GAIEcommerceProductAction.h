/*!
 @header    GAIProductAction.h
 @abstract  Google Analytics iOS SDK Hit Format Header
 @copyright Copyright 2014 Google Inc. All rights reserved.
 */

#import <Foundation/Foundation.h>

/*!
 * Class to construct transaction/checkout or other product interaction related information for a
 * Google Analytics hit. Use this class to report information about products sold, viewed or
 * refunded. This class is intended to be used with GAIDictionaryBuilder.
 * <br>
 * Typical usage:
 * <code>
 * [tracker set:kGAIScreenName value:@"MyScreen"];
 * GAIDictionaryBuilder *builder = [GAIDictionaryBuilder createScreenView];
 * GAIEcommerceProductAction *action = [[GAIEcommerceProductAction alloc] init];
 * [action setAction:kGAIPAPurchase];
 * [action setTransactionId:@"TT-1234"];
 * [action setRevenue:@3.14];
 * [action setCouponCode:@"EXTRA100"];
 * [builder setProductAction:action];
 * GAIEcommerceProduct *product = [[GAIEcommerceProduct alloc] init];
 * [product setId:@""PID-1234""];
 * [product setName:@"Space Monkeys!"];
 * [product setPrice:@100];
 * [product setQuantity:@2];
 * [builder addProduct:product];
 * [tracker send:[builder build]];
 * </code>
 */
@interface GAIEcommerceProductAction : NSObject

/*!
 Sets the product action field for this product action. Valid values can be found in
 GAIEcommerceFields.h under "product action values".
 */
- (GAIEcommerceProductAction *)setAction:(NSString *)productAction;

/*!
 The unique id associated with the transaction.  This value is used for kGAIPAPurchase and
 kGAIPARefund product actions.
 */
- (GAIEcommerceProductAction *)setTransactionId:(NSString *)transactionId;

/*!
 Sets the transaction's affiliation value.  This value is used for kGAIPAPurchase and
 kGAIPARefund product actions.
 */
- (GAIEcommerceProductAction *)setAffiliation:(NSString *)affiliation;

/*!
 Sets the transaction's total revenue.  This value is used for kGAIPAPurchase and kGAIPARefund
 product actions.
 */
- (GAIEcommerceProductAction *)setRevenue:(NSNumber *)revenue;

/*!
 Sets the transaction's total tax.  This value is used for kGAIPAPurchase and kGAIPARefund
 product actions.
 */
- (GAIEcommerceProductAction *)setTax:(NSNumber *)tax;

/*!
 Sets the transaction's total shipping costs.  This value is used for kGAIPAPurchase and
 kGAIPARefund product actions.
 */
- (GAIEcommerceProductAction *)setShipping:(NSNumber *)shipping;

/*!
 Sets the coupon code used in this transaction.  This value is used for kGAIPAPurchase and
 kGAIPARefund product actions.
 */
- (GAIEcommerceProductAction *)setCouponCode:(NSString *)couponCode;

/*!
 Sets the checkout process's progress.  This value is used for kGAICheckout and
 kGAICheckoutOptions product actions.
 */
- (GAIEcommerceProductAction *)setCheckoutStep:(NSNumber *)checkoutStep;

/*!
 Sets the option associated with the checkout.  This value is used for kGAICheckout and
 kGAICheckoutOptions product actions.
 */
- (GAIEcommerceProductAction *)setCheckoutOption:(NSString *)checkoutOption;

/*!
 Sets the list name associated with the products in Google Analytics beacons.  This value is
 used in kGAIPADetail and kGAIPAClick product actions.
 */
- (GAIEcommerceProductAction *)setProductActionList:(NSString *)productActionList;

/*!
 Sets the list source name associated with the products in Google Analytics beacons.  This value
 is used in kGAIPADetail and kGAIPAClick product actions.
 */
- (GAIEcommerceProductAction *)setProductListSource:(NSString *)productListSource;

/*!
 Builds an NSDictionary of fields stored in this instance representing this product action.
 <br>
 Normally, users will have no need to call this method.
 */
- (NSDictionary *)build;
@end
