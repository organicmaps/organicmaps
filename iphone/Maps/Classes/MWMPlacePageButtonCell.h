#import "MWMTableViewCell.h"
#import "MWMPlacePageEntity.h"

@class MWMPlacePageViewManager;

@interface MWMPlacePageButtonCell : MWMTableViewCell

- (void)config:(MWMPlacePageViewManager *)manager forType:(MWMPlacePageCellType)type;

@end
