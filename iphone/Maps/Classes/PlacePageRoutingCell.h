
#import <UIKit/UIKit.h>

@class PlacePageRoutingCell;
@protocol PlacePageRoutingCellDelegate <NSObject>

- (void)routeCellDidSetStartPoint:(PlacePageRoutingCell *)cell;
- (void)routeCellDidSetEndPoint:(PlacePageRoutingCell *)cell;

@end

@interface PlacePageRoutingCell : UITableViewCell

@property (nonatomic, weak) id <PlacePageRoutingCellDelegate> delegate;

+ (CGFloat)cellHeight;

@end
