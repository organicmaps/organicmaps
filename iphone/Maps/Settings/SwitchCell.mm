
#import "SwitchCell.h"

@implementation SwitchCell

- (void)awakeFromNib
{
  [self.switchButton addTarget:self action:@selector(switchChanged:) forControlEvents:UIControlEventValueChanged];
}

- (void)switchChanged:(UISwitch *)sender
{
  [self.delegate switchCell:self didChangeValue:sender.on];
}

@end
