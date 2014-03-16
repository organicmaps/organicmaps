
#import <UIKit/UIKit.h>

typedef NS_ENUM(NSUInteger, SearchCellPosition) {
  SearchCellPositionFirst = 1,
  SearchCellPositionMiddle = 2,
  SearchCellPositionLast = 3,
  SearchCellPositionAlone = 4,
};

@interface SearchCell : UITableViewCell

@property (nonatomic) SearchCellPosition position;

@end
