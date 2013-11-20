
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

- (void)setSelected:(BOOL)selected animated:(BOOL)animated
{
    [super setSelected:selected animated:animated];

    // Configure the view for the selected state
}

@end
