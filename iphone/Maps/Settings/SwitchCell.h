
#import <UIKit/UIKit.h>

@class SwitchCell;
@protocol SwitchCellDelegate <NSObject>

- (void)switchCell:(SwitchCell *)cell didChangeValue:(BOOL)value;

@end

@interface SwitchCell : UITableViewCell

@property (retain, nonatomic) IBOutlet UILabel * titleLabel;
@property (retain, nonatomic) IBOutlet UISwitch * switchButton;

@property (weak) id <SwitchCellDelegate> delegate;

@end
