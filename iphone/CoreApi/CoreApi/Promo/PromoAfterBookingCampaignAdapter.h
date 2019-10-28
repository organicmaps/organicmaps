#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface PromoAfterBookingCampaignAdapter : NSObject

@property(nonatomic, readonly) NSString *promoId;
@property(nonatomic, readonly) NSString *promoUrl;
@property(nonatomic, readonly) NSString *pictureUrl;

- (nullable instancetype)init;

@end

NS_ASSUME_NONNULL_END
