#import "MWMTableViewCell.h"

@class MWMPlacePage;

@interface MWMPlacePageBookmarkCell : UITableViewCell

- (void)config:(MWMPlacePage *)placePage forHeight:(BOOL)forHeight;

- (CGFloat)cellHeight;

@end
