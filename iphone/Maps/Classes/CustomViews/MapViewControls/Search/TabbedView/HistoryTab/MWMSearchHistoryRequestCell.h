#import "MWMTableViewCell.h"

@interface MWMSearchHistoryRequestCell : MWMTableViewCell

- (void)config:(NSString *)title;

+ (CGFloat)defaultCellHeight;
- (CGFloat)cellHeight;

@end
