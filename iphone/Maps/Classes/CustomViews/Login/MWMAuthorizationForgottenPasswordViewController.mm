#import "MWMAuthorizationForgottenPasswordViewController.h"
#import "UIColor+MapsMeColor.h"
#import "UITextField+RuntimeAttributes.h"

@interface MWMAuthorizationForgottenPasswordViewController () <UITextFieldDelegate>

@property (weak, nonatomic) IBOutlet UITextField * emailTextField;
@property (weak, nonatomic) IBOutlet UIButton * resetPasswordButton;

@property (nonatomic) BOOL isCorrect;

@end

@implementation MWMAuthorizationForgottenPasswordViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"forgot_password").capitalizedString;
  self.isCorrect = NO;
}

- (BOOL)shouldAutorotate
{
  return NO;
}

#pragma mark - UITextFieldDelegate

- (BOOL)textField:(UITextField *)textField
shouldChangeCharactersInRange:(NSRange)range
replacementString:(NSString *)string
{
  NSString * newString =
  [textField.text stringByReplacingCharactersInRange:range withString:string];
  self.isCorrect = [textField.validator validateString:newString];

  return YES;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  [self resetPassword];
  return YES;
}

#pragma mark - Actions

- (IBAction)resetPassword
{
  if (!self.resetPasswordButton.enabled)
    return;
  // TODO: Add password recovery
  [self cancel];
}

- (IBAction)cancel
{
  [self.navigationController popViewControllerAnimated:YES];
}

#pragma mark - Properties

- (void)setIsCorrect:(BOOL)isCorrect
{
  _isCorrect = isCorrect;
  self.resetPasswordButton.enabled = isCorrect;
  CALayer * layer = self.resetPasswordButton.layer;
  layer.borderColor = (isCorrect ? [UIColor buttonEnabledBlueText] : [UIColor clearColor]).CGColor;
}

@end
