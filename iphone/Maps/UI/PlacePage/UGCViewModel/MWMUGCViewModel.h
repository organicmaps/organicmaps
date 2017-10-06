#import "MWMReviewsViewModelProtocol.h"

typedef NS_ENUM(NSUInteger, MWMUGCViewModelPrice) {
  MWMUGCViewModelPriceLow,
  MWMUGCViewModelPriceMedium,
  MWMUGCViewModelPriceHigh
};

namespace place_page
{
class Info;
}

@class MWMUGCRatingValueType;
@class MWMUGCRatingStars;
@class MWMUGCYourReview;
@class MWMUGCReview;
@protocol MWMReviewsViewModelProtocol;

@interface MWMUGCViewModel : NSObject<MWMReviewsViewModelProtocol>

- (instancetype)initWithInfo:(place_page::Info const &)info refresh:(MWMVoidBlock)refresh;

- (BOOL)isAvailable;
- (BOOL)canAddReview;
- (BOOL)canAddTextToReview;
- (BOOL)isYourReviewAvailable;
- (BOOL)isReviewsAvailable;

- (NSInteger)ratingCellsCount;
- (NSInteger)addReviewCellsCount;

- (NSInteger)totalReviewsCount;
- (MWMUGCRatingValueType *)summaryRating;
- (NSArray<MWMUGCRatingStars *> *)ratings;

@end
