#import "MWMSearchCell.h"

@interface MWMSearchSuggestionCell : MWMSearchCell

@property (nonatomic) BOOL isLightTheme;
@property (nonatomic) BOOL isLastCell;

+ (CGFloat)cellHeight;

@end
