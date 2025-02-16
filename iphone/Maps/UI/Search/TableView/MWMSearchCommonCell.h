#import "MWMSearchCell.h"

@class SearchResult;

NS_SWIFT_NAME(SearchCommonCell)
@interface MWMSearchCommonCell : MWMSearchCell

- (void)configureWith:(SearchResult * _Nonnull)result isPartialMatching:(BOOL)isPartialMatching;

@end
