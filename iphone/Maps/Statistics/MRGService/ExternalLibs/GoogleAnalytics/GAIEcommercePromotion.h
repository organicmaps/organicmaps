/*!
 @header    GAIEcommercePromotion.h
 @abstract  Google Analytics iOS SDK Hit Format Header
 @copyright Copyright 2014 Google Inc. All rights reserved.
 */

#import <Foundation/Foundation.h>

/*!
 * Class to construct promotion related fields for Google Analytics hits. The fields from this class
 * can be used to represent internal promotions that run within an app, such as banners, banner ads
 * etc.
 *
 * Typical usage:
 * <code>
 * GAIDictionaryBuilder *builder = [GAIDictionaryBuilder createScreenView];
 * GAIEcommercePromotion *promotion = [[GAIEcommercePromotion alloc] init];
 * [promotion setId:@"PROMO-ID1234"];
 * [promotion setName:@"Home screen banner"];
 * [builder set:kGAIPromotionClick forKey:kGAIPromotionAction];
 * [builder addPromotion:promotion];
 * [tracker send:builder.build]];
 * </code>
 */
@interface GAIEcommercePromotion : NSObject

/*!
 Sets the id that is used to identify a promotion in GA reports.
 */
- (GAIEcommercePromotion *)setId:(NSString *)pid;

/*!
 Sets the name that is used to identify a promotion in GA reports.
 */
- (GAIEcommercePromotion *)setName:(NSString *)name;

/*!
 Sets the name of the creative associated with the promotion.
 */
- (GAIEcommercePromotion *)setCreative:(NSString *)creative;

/*!
 Sets the position of the promotion.
 */
- (GAIEcommercePromotion *)setPosition:(NSString *)position;

/*!
 Builds an NSDictionary of fields stored in this instance.  The index parameter is the
 index of this promotion in that promotion list.
 <br>
 Normally, users will have no need to call this method.
 */
- (NSDictionary *)buildWithIndex:(NSUInteger)index;
@end
