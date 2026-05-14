#import "MWMEditorTextTableViewCell.h"
#import "MWMEditorCommon.h"
#import "SwiftBridge.h"
#import "UIImageView+Coloring.h"

@interface MWMEditorTextTableViewCell () <UITextFieldDelegate>

@property(weak, nonatomic) IBOutlet UIImageView * icon;
@property(weak, nonatomic, readwrite) IBOutlet UITextField * textField;
@property(weak, nonatomic) IBOutlet UILabel * errorLabel;

@property(weak, nonatomic) id<MWMEditorCellProtocol> delegate;

@property(nonatomic) BOOL isValid;

@end

@implementation MWMEditorTextTableViewCell

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
               placeholder:(NSString *)placeholder
              keyboardType:(UIKeyboardType)keyboardType
            capitalization:(UITextAutocapitalizationType)capitalization
{
  [self configWithDelegate:delegate
                      icon:icon
                      text:text
               placeholder:placeholder
              errorMessage:nil
                   isValid:YES
              keyboardType:keyboardType
            capitalization:capitalization];
}

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
               placeholder:(NSString *)placeholder
              errorMessage:(NSString *)errorMessage
                   isValid:(BOOL)isValid
              keyboardType:(UIKeyboardType)keyboardType
            capitalization:(UITextAutocapitalizationType)capitalization
{
  self.delegate = delegate;
  self.icon.image = icon;
  [self.icon setStyleNameAndApply:@"MWMBlack"];
  self.icon.hidden = (icon == nil);

  self.textField.text = text;
  [self.textField setStyleNameAndApply:@"regular17:blackPrimaryText"];
  self.textField.attributedPlaceholder =
      [[NSAttributedString alloc] initWithString:placeholder
                                      attributes:@{
                                        NSForegroundColorAttributeName: [UIColor blackHintText],
                                        NSFontAttributeName: self.textField.font ?: UIFont.regular17.dynamic
                                      }];
  self.errorLabel.text = errorMessage;
  [self.errorLabel applyTheme];
  self.textField.keyboardType = keyboardType;
  self.textField.backgroundColor = UIColor.clearColor;
  self.isValid = isValid;
  self.textField.autocapitalizationType = capitalization;
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

@end
