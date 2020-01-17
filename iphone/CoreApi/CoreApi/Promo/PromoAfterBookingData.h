#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface PromoAfterBookingData : NSObject

@property(nonatomic, readonly) NSString *promoId;
@property(nonatomic, readonly) NSString *promoUrl;
@property(nonatomic, readonly) NSString *pictureUrl;
@property(nonatomic, readonly) BOOL enabled;

@end

NS_ASSUME_NONNULL_END
