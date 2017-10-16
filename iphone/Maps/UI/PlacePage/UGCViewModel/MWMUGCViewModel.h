#import "MWMReviewsViewModelProtocol.h"

typedef NS_ENUM(NSUInteger, MWMUGCViewModelPrice) {
  MWMUGCViewModelPriceLow,
  MWMUGCViewModelPriceMedium,
  MWMUGCViewModelPriceHigh
};

namespace ugc
{
struct UGC;
struct UGCUpdate;
}

@class MWMUGCRatingValueType;
@class MWMUGCRatingStars;
@class MWMUGCYourReview;
@class MWMUGCReview;
@protocol MWMReviewsViewModelProtocol;

@interface MWMUGCViewModel : NSObject<MWMReviewsViewModelProtocol>

- (instancetype)initWithUGC:(ugc::UGC const &)ugc update:(ugc::UGCUpdate const &)update;

- (BOOL)isUGCEmpty;
- (BOOL)isUGCUpdateEmpty;

- (NSUInteger)ratingCellsCount;
- (NSUInteger)addReviewCellsCount;

- (NSUInteger)totalReviewsCount;
- (MWMUGCRatingValueType *)summaryRating;
- (NSArray<MWMUGCRatingStars *> *)ratings;

@end
