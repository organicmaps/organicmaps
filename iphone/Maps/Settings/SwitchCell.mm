#import "SwitchCell.h"
#import "UIColor+MapsMeColor.h"

@implementation SwitchCell

- (void)awakeFromNib
{
  [self.switchButton addTarget:self action:@selector(switchChanged:) forControlEvents:UIControlEventValueChanged];
  self.backgroundColor = [UIColor white];
}

- (void)switchChanged:(UISwitch *)sender
{
  [self.delegate switchCell:self didChangeValue:sender.on];
}

@end
