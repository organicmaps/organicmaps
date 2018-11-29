#import "MWMRatingSummaryViewValueType.h"
#import "MWMUGCReviewSource.h"

@interface MWMPlacePageManagerHelper : NSObject

+ (void)updateAvailableArea:(CGRect)frame;
+ (void)showUGCAddReview:(MWMRatingSummaryViewValueType)value fromSource:(MWMUGCReviewSource)source;
+ (void)searchSimilar;

@end
