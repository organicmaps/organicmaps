#import "MWMEditorAdditionalNameTableViewCell.h"
#import "SwiftBridge.h"

@interface MWMEditorAdditionalNameTableViewCell ()

@property(weak, nonatomic) IBOutlet UIStackView * stackView;
@property(weak, nonatomic) IBOutlet UILabel * languageLabel;
@property(weak, nonatomic) IBOutlet UITextField * textField;
@property(weak, nonatomic) IBOutlet UILabel * errorLabel;

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

- (void)processValidation
{
  self.errorLabel.hidden = self.isValid || self.errorLabel.text.length == 0;
  [self.contentView setStyleNameAndApply:self.isValid ? @"Background" : @"ErrorBackground"];
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

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event
{
  // Allow to tap on the whole cell to start editing.
  UIView * view = [super hitTest:point withEvent:event];
  if (view == self.stackView)
    return self.textField;
  return view;
}

@end
