#import "MWMEditorAdditionalNameTableViewCell.h"

@interface MWMEditorAdditionalNameTableViewCell ()

@property(weak, nonatomic) IBOutlet UILabel * languageLabel;
@property(weak, nonatomic) IBOutlet UIButton * languageButton;
@property(weak, nonatomic) IBOutlet UITextField * textField;

@property(nonatomic, readwrite) NSInteger code;

@property(weak, nonatomic) id<MWMEditorAdditionalName> delegate;

@end

@implementation MWMEditorAdditionalNameTableViewCell

- (void)configWithDelegate:(id<MWMEditorAdditionalName>)delegate
                  langCode:(NSInteger)langCode
                  langName:(NSString *)langName
                      name:(NSString *)name
              keyboardType:(UIKeyboardType)keyboardType
{
  self.delegate = delegate;
  self.code = langCode;
  self.languageLabel.text = langName;
  [self.languageButton setTitle:langName forState:UIControlStateNormal];
  self.textField.text = name;
  self.textField.keyboardType = keyboardType;
}

- (IBAction)changeLanguageTap { [self.delegate editAdditionalNameLanguage:self.code]; }
#pragma mark - UITextFieldDelegate

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
