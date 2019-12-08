#import <CoreApi/UgcSummaryRatingType.h>

NS_ASSUME_NONNULL_BEGIN

@interface MWMDiscoveryHotelViewModel : NSObject

@property(nonatomic, readonly) NSString *title;
@property(nonatomic, readonly) NSString *subtitle;
@property(nonatomic, readonly) NSString *price;
@property(nonatomic, readonly) NSString *distance;
@property(nonatomic, readonly) BOOL isPopular;
@property(nonatomic, readonly) NSString *ratingValue;
@property(nonatomic, readonly) UgcSummaryRatingType ratingType;

- (instancetype)initWithTitle:(NSString *)title
                     subtitle:(NSString *)subtitle
                        price:(NSString *)price
                     distance:(NSString *)distance
                    isPopular:(BOOL)isPopular
                  ratingValue:(NSString *) ratingValue
                   ratingType:(UgcSummaryRatingType)ratingType;

@end

NS_ASSUME_NONNULL_END
