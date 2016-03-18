#import "MWMEditorCommon.h"
#import "MWMEditorTextTableViewCell.h"
#import "UIColor+MapsMeColor.h"
#import "UIImageView+Coloring.h"
#import "UITextField+RuntimeAttributes.h"

@interface MWMEditorTextTableViewCell () <UITextFieldDelegate>

@property (weak, nonatomic) IBOutlet UIImageView * icon;
@property (weak, nonatomic) IBOutlet UITextField * textField;

@property (weak, nonatomic) id<MWMEditorCellProtocol> delegate;

@end

@implementation MWMEditorTextTableViewCell

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
               placeholder:(NSString *)placeholder
              keyboardType:(UIKeyboardType)keyboardType
{
  self.delegate = delegate;
  self.icon.image = icon;
  self.icon.mwm_coloring = MWMImageColoringBlack;
  self.icon.hidden = (icon == nil);
  self.textField.text = text;
  self.textField.attributedPlaceholder = [[NSAttributedString alloc] initWithString:placeholder attributes:
                                          @{NSForegroundColorAttributeName : [UIColor blackHintText]}];
  self.textField.keyboardType = keyboardType;
}

#pragma mark - UITextFieldDelegate

- (BOOL)textFieldShouldBeginEditing:(UITextField *)textField
{
  [self.delegate cellBeginEditing:self];
  return YES;
}

- (BOOL)textField:(UITextField *)textField
    shouldChangeCharactersInRange:(NSRange)range
                replacementString:(NSString *)string
{
  NSString * newString =
      [textField.text stringByReplacingCharactersInRange:range withString:string];
  BOOL const isCorrect = [textField.validator validateString:newString];
  if (isCorrect)
    [self.delegate cell:self changeText:newString];
  return YES;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  [textField resignFirstResponder];
  return YES;
}

#pragma mark - Properties

- (NSString *)text
{
  return self.textField.text;
}

@end
