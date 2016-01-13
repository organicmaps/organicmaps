#import "MWMTableViewCell.h"

@class SwitchCell;
@protocol SwitchCellDelegate <NSObject>

- (void)switchCell:(SwitchCell *)cell didChangeValue:(BOOL)value;

@end

@interface SwitchCell : MWMTableViewCell

@property (weak, nonatomic) IBOutlet UILabel * titleLabel;
@property (weak, nonatomic) IBOutlet UISwitch * switchButton;

@property (weak) id<SwitchCellDelegate> delegate;

@end
