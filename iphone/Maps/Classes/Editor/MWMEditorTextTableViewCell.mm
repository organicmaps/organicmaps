#import "MWMEditorCommon.h"
#import "MWMEditorTextTableViewCell.h"
#import "UIColor+MapsMeColor.h"
#import "UIImageView+Coloring.h"
#import "UITextField+RuntimeAttributes.h"

@interface MWMEditorTextTableViewCell () <UITextFieldDelegate>

@property (weak, nonatomic) IBOutlet UIImageView * icon;
@property (weak, nonatomic, readwrite) IBOutlet UITextField * textField;
@property (weak, nonatomic) IBOutlet UILabel * errorLabel;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * errorLabelTopSpace;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * labelHeight;

@property (weak, nonatomic) id<MWMEditorCellProtocol> delegate;

@property (nonatomic) BOOL isValid;

@end

@implementation MWMEditorTextTableViewCell

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
               placeholder:(NSString *)placeholder
              keyboardType:(UIKeyboardType)keyboardType
{
  [self configWithDelegate:delegate icon:icon text:text placeholder:placeholder
                            errorMessage:nil isValid:YES keyboardType:keyboardType];
}

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
               placeholder:(NSString *)placeholder
              errorMessage:(NSString *)errorMessage
                   isValid:(BOOL)isValid
              keyboardType:(UIKeyboardType)keyboardType
{
  self.delegate = delegate;
  self.icon.image = icon;
  self.icon.mwm_coloring = MWMImageColoringBlack;
  self.icon.hidden = (icon == nil);

  self.textField.text = text;
  self.textField.attributedPlaceholder = [[NSAttributedString alloc] initWithString:placeholder attributes:
                                          @{NSForegroundColorAttributeName : [UIColor blackHintText]}];
  self.errorLabel.text = errorMessage;
  self.textField.keyboardType = keyboardType;
  self.textField.backgroundColor = [UIColor clearColor];
  self.isValid = isValid;
  [self processValidation];
}

- (void)processValidation
{
  if (self.isValid)
  {
    self.labelHeight.priority = UILayoutPriorityDefaultHigh;
    self.errorLabelTopSpace.constant = 0;
    self.contentView.backgroundColor = [UIColor white];
  }
  else
  {
    self.labelHeight.priority = UILayoutPriorityDefaultLow;
    self.errorLabelTopSpace.constant = 4;
    self.contentView.backgroundColor = [UIColor errorPink];
  }
  [self layoutIfNeeded];
}

#pragma mark - UITextFieldDelegate

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
  if (!self.isValid)
  {
    self.isValid = YES;
    [self processValidation];
    [self.delegate tryToChangeInvalidStateForCell:self];
  }
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

#pragma mark - Properties

- (NSString *)text
{
  return self.textField.text;
}

@end
