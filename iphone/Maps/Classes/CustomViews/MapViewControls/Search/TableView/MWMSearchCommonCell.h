#import "MWMSearchCell.h"

#include "search/result.hpp"

@interface MWMSearchCommonCell : MWMSearchCell

+ (CGFloat)defaultCellHeight;
- (CGFloat)cellHeight;

- (void)config:(search::Result &)result forHeight:(BOOL)forHeight;

@end
