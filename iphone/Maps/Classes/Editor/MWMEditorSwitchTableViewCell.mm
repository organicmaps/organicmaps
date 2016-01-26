#import "MWMEditorSwitchTableViewCell.h"
#import "UIImageView+Coloring.h"

@interface MWMEditorSwitchTableViewCell ()

@property (weak, nonatomic) IBOutlet UIImageView * icon;
@property (weak, nonatomic) IBOutlet UILabel * label;
@property (weak, nonatomic) IBOutlet UISwitch * switchControl;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * bottomSeparatorLeadingOffset;

@property (weak, nonatomic) id<MWMEditorCellProtocol> delegate;

@end

@implementation MWMEditorSwitchTableViewCell

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
                        on:(BOOL)on
                  lastCell:(BOOL)lastCell
{
  self.delegate = delegate;
  self.icon.image = icon;
  self.icon.mwm_coloring = MWMImageColoringBlack;
  self.label.text = text;
  self.switchControl.on = on;
  self.bottomSeparatorLeadingOffset.priority = lastCell ? UILayoutPriorityDefaultHigh : UILayoutPriorityDefaultLow;
}

- (IBAction)valueChanged
{
  [self.delegate cell:self changeSwitch:self.switchControl.on];
}

@end
