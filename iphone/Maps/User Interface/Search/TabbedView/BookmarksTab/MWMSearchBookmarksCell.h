#import "MWMTableViewCell.h"

@interface MWMSearchBookmarksCell : MWMTableViewCell

- (void)configForIndex:(NSInteger)index;

+ (CGFloat)defaultCellHeight;
- (CGFloat)cellHeight;

@end
