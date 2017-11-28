#import "MWMEditorAdditionalNameTableViewCell.h"

namespace
{
CGFloat const kErrorLabelHeight = 16;
}  // namespace

@interface MWMEditorAdditionalNameTableViewCell ()

@property(weak, nonatomic) IBOutlet UILabel * languageLabel;
@property(weak, nonatomic) IBOutlet UITextField * textField;
@property(weak, nonatomic) IBOutlet UILabel * errorLabel;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * errorLabelHeight;

@property(nonatomic, readwrite) NSInteger code;

@property(weak, nonatomic) id<MWMEditorAdditionalName> delegate;

@property(nonatomic) BOOL isValid;

@end

@implementation MWMEditorAdditionalNameTableViewCell

- (void)configWithDelegate:(id<MWMEditorAdditionalName>)delegate
                  langCode:(NSInteger)langCode
                  langName:(NSString *)langName
                      name:(NSString *)name
              errorMessage:(NSString *)errorMessage
                   isValid:(BOOL)isValid
              keyboardType:(UIKeyboardType)keyboardType
{
  self.delegate = delegate;
  self.code = langCode;
  self.languageLabel.text = langName;
  self.errorLabel.text = errorMessage;
  self.isValid = isValid;
  self.textField.text = name;
  self.textField.keyboardType = keyboardType;
  self.textField.backgroundColor = [UIColor clearColor];
  [self processValidation];
}

- (IBAction)changeLanguageTap { [self.delegate editAdditionalNameLanguage:self.code]; }

- (void)processValidation
{
  if (self.isValid)
  {
    self.errorLabelHeight.constant = 0;
    self.contentView.backgroundColor = [UIColor white];
  }
  else
  {
    self.errorLabelHeight.constant = kErrorLabelHeight;
    self.contentView.backgroundColor = [UIColor errorPink];
  }
  [self layoutIfNeeded];
}

- (void)changeInvalidCellState
{
  if (self.isValid)
    return;
  self.isValid = YES;
  [self processValidation];
  [self.delegate tryToChangeInvalidStateForCell:self];
}

#pragma mark - UITextFieldDelegate

- (BOOL)textField:(UITextField *)textField
    shouldChangeCharactersInRange:(NSRange)range
                replacementString:(NSString *)string
{
  [self changeInvalidCellState];
  return YES;
}

- (BOOL)textFieldShouldClear:(UITextField *)textField
{
  [self changeInvalidCellState];
  return YES;
}

- (void)textFieldDidEndEditing:(UITextField *)textField
{
  [self.delegate cell:self changedText:textField.text];
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  [textField resignFirstResponder];
  return YES;
}

@end
