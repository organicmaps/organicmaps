#import "MWMEditorSwitchTableViewCell.h"
#import "UIImageView+Coloring.h"

@interface MWMEditorSwitchTableViewCell ()

@property (weak, nonatomic) IBOutlet UIImageView * icon;
@property (weak, nonatomic) IBOutlet UILabel * label;
@property (weak, nonatomic) IBOutlet UISwitch * switchControl;

@property (weak, nonatomic) id<MWMEditorCellProtocol> delegate;

@end

@implementation MWMEditorSwitchTableViewCell

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
                        on:(BOOL)on
{
  self.delegate = delegate;
  self.icon.image = icon;
  self.icon.mwm_coloring = MWMImageColoringBlack;
  self.label.text = text;
  self.switchControl.on = on;
}

- (IBAction)valueChanged
{
  [self.delegate cell:self changeSwitch:self.switchControl.on];
}

@end
