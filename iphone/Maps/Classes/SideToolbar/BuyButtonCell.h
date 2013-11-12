
#import <UIKit/UIKit.h>

@class BuyButtonCell;
@protocol BuyButtonCellDelegate <NSObject>

- (void)buyButtonCellDidPressBuyButton:(BuyButtonCell *)cell;

@end

@interface BuyButtonCell : UITableViewCell

@property (weak) id <BuyButtonCellDelegate> delegate;

+ (CGFloat)cellHeight;

@end
