
#import <UIKit/UIKit.h>

@class PlacePageShareCell;
@protocol PlacePageShareCellDelegate <NSObject>

- (void)shareCellDidPressShareButton:(PlacePageShareCell *)cell;
- (void)shareCellDidPressApiButton:(PlacePageShareCell *)cell;

@end

@interface PlacePageShareCell : UITableViewCell

@property (nonatomic, weak) id <PlacePageShareCellDelegate> delegate;

@property (nonatomic) NSString * apiAppTitle;

+ (CGFloat)cellHeight;

@end
