#import "MWMTableViewCell.h"

@protocol MWMButtonCellDelegate<NSObject>

- (void)cellSelect:(UITableViewCell *)cell;

@end

@interface MWMButtonCell : MWMTableViewCell

- (void)configureWithDelegate:(id<MWMButtonCellDelegate>)delegate title:(NSString *)title;

@end
