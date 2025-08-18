#import "MWMTableViewCell.h"

NS_ASSUME_NONNULL_BEGIN

@protocol MWMButtonCellDelegate <NSObject>

- (void)cellDidPressButton:(UITableViewCell *)cell;

@end

@interface MWMButtonCell : MWMTableViewCell

- (void)configureWithDelegate:(id<MWMButtonCellDelegate>)delegate title:(NSString *)title enabled:(BOOL)enabled;

@end

NS_ASSUME_NONNULL_END
