#import "MWMReviewsViewModelProtocol.h"

#include <vector>

typedef NS_ENUM(NSUInteger, MWMUGCViewModelPrice) {
  MWMUGCViewModelPriceLow,
  MWMUGCViewModelPriceMedium,
  MWMUGCViewModelPriceHigh
};

namespace ugc
{
struct UGC;
struct UGCUpdate;

namespace view_model
{
enum class ReviewRow
{
  YourReview,
  Review,
  MoreReviews
};
}  // namespace view_model
}  // namespace ugc

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
- (std::vector<ugc::view_model::ReviewRow> const &)reviewRows;

- (NSUInteger)totalReviewsCount;
- (MWMUGCRatingValueType *)summaryRating;
- (NSArray<MWMUGCRatingStars *> *)ratings;

@end
