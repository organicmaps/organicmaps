#import "AddSetTableViewCell.h"

@interface AddSetTableViewCell () <UITextFieldDelegate>

@end

@implementation AddSetTableViewCell

- (void)awakeFromNib
{
  self.textField.placeholder = L(@"bookmark_set_name");
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
