#import <Foundation/Foundation.h>

#import "UgcSummaryRatingType.h"

NS_ASSUME_NONNULL_BEGIN

@interface UgcSummaryRating : NSObject

@property(nonatomic, readonly) NSString *ratingString;
@property(nonatomic, readonly) UgcSummaryRatingType ratingType;

- (instancetype)initWithRating:(float)ratingValue;

@end

NS_ASSUME_NONNULL_END
