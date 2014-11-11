/*!
 @header    GAIDictionaryBuilder.h
 @abstract  Google Analytics iOS SDK Hit Format Header
 @copyright Copyright 2013 Google Inc. All rights reserved.
 */

#import <Foundation/Foundation.h>

#import "GAIEcommerceProduct.h"
#import "GAIEcommerceProductAction.h"
#import "GAIEcommercePromotion.h"

/*!
 * Helper class to build a dictionary of hit parameters and values.
 * <br>
 * Examples:
 * <code>
 * id<GAITracker> t = // get a tracker.
 * [t send:[[[GAIDictionaryBuilder createEventWithCategory:@"EventCategory"
 *                                                  action:@"EventAction"
 *                                                   label:nil
 *                                                   value:nil]
 *     set:@"dimension1" forKey:[GAIFields customDimensionForIndex:1]] build]];
 * </code>
 * This will send an event hit type with the specified parameters
 * and a custom dimension parameter.
 * <br>
 * If you want to send a parameter with all hits, set it on GAITracker directly.
 * <code>
 * [t set:kGAIScreenName value:@"Home"];
 * [t send:[[GAIDictionaryBuilder createSocialWithNetwork:@"Google+"
 *                                                 action:@"PlusOne"
 *                                                 target:@"SOME_URL"] build]];
 * [t send:[[GAIDictionaryBuilder createSocialWithNetwork:@"Google+"
 *                                                 action:@"Share"
 *                                                 target:@"SOME_POST"] build]];
 * [t send:[[GAIDictionaryBuilder createSocialWithNetwork:@"Google+"
 *                                                 action:@"HangOut"
 *                                                 target:@"SOME_CIRCLE"]
 *     build]];
 * </code>
 * You can override a value set on the tracker by adding it to the dictionary.
 * <code>
 * [t set:kGAIScreenName value:@"Home"];
 * [t send:...];
 * [t send[[[GAIDictionaryBuilder createEventWithCategory:@"click"
 *                                                 action:@"popup"
 *                                                  label:nil
 *                                                  value:nil]
 *     set:@"popup title" forKey:kGAIScreenName] build]];
 * </code>
 * The values set via [GAIDictionaryBuilder set] or
 * [GAIDictionaryBuilder setAll] will override any existing values in the
 * GAIDictionaryBuilder object (i.e. initialized by
 * [GAIDictionaryBuilder createXYZ]). e.g.
 * <code>
 * GAIDictionaryBuilder *m =
 *     GAIDictionaryBuilder createTimingWithCategory:@"category"
 *                                          interval:@0
 *                                              name:@"name"
 *                                             label:nil];
 * [t send:[m.set:@"10" forKey:kGAITimingVar] build];
 * [t send:[m.set:@"20" forKey:kGAITimingVar] build];
 * </code>
 */
@interface GAIDictionaryBuilder : NSObject

- (GAIDictionaryBuilder *)set:(NSString *)value
                       forKey:(NSString *)key;

/*!
 * Copies all the name-value pairs from params into this object, ignoring any
 * keys that are not NSString and any values that are neither NSString or
 * NSNull.
 */
- (GAIDictionaryBuilder *)setAll:(NSDictionary *)params;

/*!
 * Returns the value for the input parameter paramName, or nil if paramName
 * is not present.
 */
- (NSString *)get:(NSString *)paramName;

/*!
 * Return an NSMutableDictionary object with all the parameters set in this
 */
- (NSMutableDictionary *)build;

/*!
 * Parses and translates utm campaign parameters to analytics campaign param
 * and returns them as a map.
 *
 * @param params url containing utm campaign parameters.
 *
 * Valid campaign parameters are:
 * <ul>
 * <li>utm_id</li>
 * <li>utm_campaign</li>
 * <li>utm_content</li>
 * <li>utm_medium</li>
 * <li>utm_source</li>
 * <li>utm_term</li>
 * <li>dclid</li>
 * <li>gclid</li>
 * <li>gmob_t</li>
 * </ul>
 * <p>
 * Example:
 * http://my.site.com/index.html?utm_campaign=wow&utm_source=source
 * utm_campaign=wow&utm_source=source.
 * <p>
 * For more information on auto-tagging, see
 * http://support.google.com/googleanalytics/bin/answer.py?hl=en&answer=55590
 * <p>
 * For more information on manual tagging, see
 * http://support.google.com/googleanalytics/bin/answer.py?hl=en&answer=55518
 */
- (GAIDictionaryBuilder *)setCampaignParametersFromUrl:(NSString *)urlString;

/*!
 Returns a GAIDictionaryBuilder object with parameters specific to an appview
 hit.

 Note that using this method will not set the screen name for followon hits.  To
 do that you need to call set:kGAIDescription value:<screenName> on the
 GAITracker instance.

 This method is deprecated.  Use createScreenView instead.
 */
+ (GAIDictionaryBuilder *)createAppView;

/*!
 Returns a GAIDictionaryBuilder object with parameters specific to a screenview
 hit.

 Note that using this method will not set the screen name for followon hits.  To
 do that you need to call set:kGAIDescription value:<screenName> on the
 GAITracker instance.
 */
+ (GAIDictionaryBuilder *)createScreenView;

/*!
 Returns a GAIDictionaryBuilder object with parameters specific to an event hit.
 */
+ (GAIDictionaryBuilder *)createEventWithCategory:(NSString *)category
                                           action:(NSString *)action
                                            label:(NSString *)label
                                            value:(NSNumber *)value;

/*!
 Returns a GAIDictionaryBuilder object with parameters specific to an exception
 hit.
 */
+ (GAIDictionaryBuilder *)createExceptionWithDescription:(NSString *)description
                                               withFatal:(NSNumber *)fatal;

/*!
 Returns a GAIDictionaryBuilder object with parameters specific to an item hit.
 */
+ (GAIDictionaryBuilder *)createItemWithTransactionId:(NSString *)transactionId
                                                 name:(NSString *)name
                                                  sku:(NSString *)sku
                                             category:(NSString *)category
                                                price:(NSNumber *)price
                                             quantity:(NSNumber *)quantity
                                         currencyCode:(NSString *)currencyCode;

/*!
 Returns a GAIDictionaryBuilder object with parameters specific to a social hit.
 */
+ (GAIDictionaryBuilder *)createSocialWithNetwork:(NSString *)network
                                           action:(NSString *)action
                                           target:(NSString *)target;

/*!
 Returns a GAIDictionaryBuilder object with parameters specific to a timing hit.
 */
+ (GAIDictionaryBuilder *)createTimingWithCategory:(NSString *)category
                                          interval:(NSNumber *)intervalMillis
                                              name:(NSString *)name
                                             label:(NSString *)label;

/*!
 Returns a GAIDictionaryBuilder object with parameters specific to a transaction
 hit.
 */
+ (GAIDictionaryBuilder *)createTransactionWithId:(NSString *)transactionId
                                      affiliation:(NSString *)affiliation
                                          revenue:(NSNumber *)revenue
                                              tax:(NSNumber *)tax
                                         shipping:(NSNumber *)shipping
                                     currencyCode:(NSString *)currencyCode;

/*!
 Set the product action field for this hit.
 */
- (GAIDictionaryBuilder *)setProductAction:(GAIEcommerceProductAction *)productAction;

/*!
 Adds a product to this hit.
 */
- (GAIDictionaryBuilder *)addProduct:(GAIEcommerceProduct *)product;

/*!
 Add a product impression to this hit.
 */
- (GAIDictionaryBuilder *)addProductImpression:(GAIEcommerceProduct *)product
                                impressionList:(NSString *)name;

/*!
 Add a promotion to this hit.
 */
- (GAIDictionaryBuilder *)addPromotion:(GAIEcommercePromotion *)promotion;
@end
