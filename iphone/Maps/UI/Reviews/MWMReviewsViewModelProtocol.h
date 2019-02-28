#import "MWMReviewProtocol.h"

@protocol MWMReviewsViewModelProtocol
- (NSInteger)numberOfReviews;
- (id<MWMReviewProtocol> _Nonnull)reviewWithIndex:(NSInteger)index;
- (BOOL)isExpanded:(id<MWMReviewProtocol> _Nonnull) review;
- (void)markExpanded:(id<MWMReviewProtocol> _Nonnull) review;
@end
