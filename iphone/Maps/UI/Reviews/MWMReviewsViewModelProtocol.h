#import "MWMReviewProtocol.h"

@protocol MWMReviewsViewModelProtocol
- (NSInteger)numberOfReviews;
- (id<MWMReviewProtocol> _Nonnull)reviewWithIndex:(NSInteger)index;
@end
