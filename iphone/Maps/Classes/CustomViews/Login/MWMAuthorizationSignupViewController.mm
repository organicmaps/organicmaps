#import "MWMAuthorizationCommon.h"
#import "MWMAuthorizationSignupViewController.h"
#import "MWMAuthorizationWebViewLoginViewController.h"
#import "UITextField+RuntimeAttributes.h"

typedef NS_OPTIONS(NSUInteger, MWMFieldCorrect)
{
  MWMFieldCorrectNO       = 0,
  MWMFieldCorrectEmail    = 1 << 0,
  MWMFieldCorrectLogin    = 1 << 1,
  MWMFieldCorrectPassword = 1 << 2,
  MWMFieldCorrectAll = MWMFieldCorrectEmail | MWMFieldCorrectLogin | MWMFieldCorrectPassword
};

@interface MWMAuthorizationSignupViewController ()  <UITextFieldDelegate>

@property (weak, nonatomic) IBOutlet UIButton * signupGoogleButton;
@property (weak, nonatomic) IBOutlet UIButton * signupFacebookButton;
@property (weak, nonatomic) IBOutlet UIButton * signupOSMButton;
@property (weak, nonatomic) IBOutlet UIButton * signupOSMSiteButton;

@property (weak, nonatomic) IBOutlet UITextField * emailTextField;
@property (weak, nonatomic) IBOutlet UITextField * loginTextField;
@property (weak, nonatomic) IBOutlet UITextField * passwordTextField;

@property (nonatomic) MWMFieldCorrect isCorrect;

@end

@implementation MWMAuthorizationSignupViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"sign_up");

  MWMAuthorizationConfigButton(self.signupGoogleButton, MWMAuthorizationButtonTypeGoogle);
  MWMAuthorizationConfigButton(self.signupFacebookButton, MWMAuthorizationButtonTypeFacebook);
  MWMAuthorizationConfigButton(self.signupOSMButton, MWMAuthorizationButtonTypeOSM);

  self.isCorrect = MWMFieldCorrectNO;
}

- (BOOL)shouldAutorotate
{
  return NO;
}

- (void)setSignupOSMButtonEnabled:(BOOL)enabled
{
  self.signupOSMButton.enabled = enabled;
  CALayer * layer = self.signupOSMButton.layer;
  layer.borderColor =
      (enabled ? MWMAuthorizationButtonBackgroundColor(MWMAuthorizationButtonTypeOSM)
               : [UIColor clearColor])
          .CGColor;
}

#pragma mark - UITextFieldDelegate

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
  NSString * newString =
  [textField.text stringByReplacingCharactersInRange:range withString:string];
  BOOL const isValid = [textField.validator validateString:newString];

  if ([textField isEqual:self.emailTextField])
  {
    if (isValid)
      self.isCorrect |= MWMFieldCorrectEmail;
    else
      self.isCorrect &= ~MWMFieldCorrectEmail;
  }
  if ([textField isEqual:self.loginTextField])
  {
    if (isValid)
      self.isCorrect |= MWMFieldCorrectLogin;
    else
      self.isCorrect &= ~MWMFieldCorrectLogin;
  }
  else if ([textField isEqual:self.passwordTextField])
  {
    if (isValid)
      self.isCorrect |= MWMFieldCorrectPassword;
    else
      self.isCorrect &= ~MWMFieldCorrectPassword;
  }

  return YES;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  if ([textField isEqual:self.emailTextField])
  {
    [self.loginTextField becomeFirstResponder];
  }
  else if ([textField isEqual:self.loginTextField])
  {
    [self.passwordTextField becomeFirstResponder];
  }
  else if ([textField isEqual:self.passwordTextField])
  {
    [textField resignFirstResponder];
    [self signupOSM];
  }
  return YES;
}

#pragma mark - Actions

- (IBAction)signupGoogle
{
  // TODO: Add signup
}

- (IBAction)signupFacebook
{
  // TODO: Add signup
}

- (IBAction)signupOSM
{
  if (!self.signupOSMButton.enabled)
    return;
  // TODO: Add signup
}

- (IBAction)signupOSMSite
{
  // TODO: Add signup
}

- (IBAction)cancel
{
  [self.navigationController popViewControllerAnimated:YES];
}

#pragma mark - Properties

- (void)setIsCorrect:(MWMFieldCorrect)isCorrect
{
  _isCorrect = isCorrect;
  [self setSignupOSMButtonEnabled:isCorrect == MWMFieldCorrectAll];
}

#pragma mark - Segue

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
  MWMAuthorizationWebViewLoginViewController * dvc = segue.destinationViewController;
  if ([self.signupGoogleButton isEqual:sender])
    dvc.authType = MWMWebViewAuthorizationTypeGoogle;
  else if ([self.signupFacebookButton isEqual:sender])
    dvc.authType = MWMWebViewAuthorizationTypeFacebook;
}

@end
