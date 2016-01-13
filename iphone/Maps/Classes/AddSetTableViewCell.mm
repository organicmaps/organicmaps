#import "AddSetTableViewCell.h"
#import "UIColor+MapsMeColor.h"

@interface AddSetTableViewCell () <UITextFieldDelegate>

@end

@implementation AddSetTableViewCell

- (void)awakeFromNib
{
  self.textField.placeholder = L(@"bookmark_set_name");
  self.textField.textColor = [UIColor blackPrimaryText];
  UILabel * label = [self.textField valueForKey:@"_placeholderLabel"];
  label.textColor = [UIColor blackHintText];
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
