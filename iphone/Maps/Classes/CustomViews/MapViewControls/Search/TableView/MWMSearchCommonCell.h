#import "MWMSearchCell.h"

#include "search/result.hpp"

@interface MWMSearchCommonCell : MWMSearchCell

@property (nonatomic) BOOL isLightTheme;

+ (CGFloat)defaultCellHeight;
- (CGFloat)cellHeight;

@end
