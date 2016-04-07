#import "MWMTableViewCell.h"
#import "MWMPlacePageEntity.h"

@class MWMPlacePage;

@interface MWMPlacePageButtonCell : MWMTableViewCell

- (void)config:(MWMPlacePage *)placePage forType:(MWMPlacePageCellType)type;

@end
