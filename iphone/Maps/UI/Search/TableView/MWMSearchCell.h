#import "MWMTableViewCell.h"

@class SearchResult;

@interface MWMSearchCell : MWMTableViewCell

- (void)configureWith:(SearchResult * _Nonnull)result;

@end
