#import "AddSetTableViewCell.h"

@interface AddSetTableViewCell ()<UITextFieldDelegate>

@end

@implementation AddSetTableViewCell

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.textField.textColor = [UIColor blackPrimaryText];
  self.textField.attributedPlaceholder = [[NSAttributedString alloc]
      initWithString:L(@"bookmark_set_name")
          attributes:@{NSForegroundColorAttributeName: [UIColor blackHintText]}];
}

#pragma mark - UITextFieldDelegate

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  if (textField.text.length == 0)
    return YES;

  [self.delegate onDone:textField.text];
  return NO;
}

@end
