#import "MWMTableViewCell.h"

@class SearchResult;

static CGFloat const kSearchCellSeparatorInset = 48;

@interface MWMSearchCell : MWMTableViewCell

- (void)configureWith:(SearchResult * _Nonnull)result isPartialMatching:(BOOL)isPartialMatching;

@end
