#import "MWMAuthorizationCommon.h"
#import "MWMAuthorizationOSMLoginViewController.h"
#import "UIColor+MapsMeColor.h"
#import "UITextField+RuntimeAttributes.h"

#include "editor/server_api.hpp"
#include "platform/platform.hpp"

typedef NS_OPTIONS(NSUInteger, MWMFieldCorrect)
{
  MWMFieldCorrectNO       = 0,
  MWMFieldCorrectLogin    = 1 << 0,
  MWMFieldCorrectPassword = 1 << 1,
  MWMFieldCorrectAll = MWMFieldCorrectLogin | MWMFieldCorrectPassword
};

@interface MWMAuthorizationOSMLoginViewController () <UITextFieldDelegate>

@property (weak, nonatomic) IBOutlet UITextField * loginTextField;
@property (weak, nonatomic) IBOutlet UITextField * passwordTextField;
@property (weak, nonatomic) IBOutlet UIButton * loginButton;
@property (weak, nonatomic) IBOutlet UIButton * forgotButton;

@property (nonatomic) MWMFieldCorrect isCorrect;

@end

@implementation MWMAuthorizationOSMLoginViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"osm_login");
  self.isCorrect = MWMFieldCorrectNO;
  [self checkConnection];
}

- (BOOL)shouldAutorotate
{
  return NO;
}

- (void)checkConnection
{
  self.forgotButton.enabled = Platform::IsConnected();
}

- (void)setLoginButtonEnabled:(BOOL)enabled
{
  self.loginButton.enabled = enabled;
  CALayer * layer = self.loginButton.layer;
  layer.borderColor = (enabled ? [UIColor buttonEnabledBlueText] : [UIColor clearColor]).CGColor;
}

#pragma mark - UITextFieldDelegate

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
  NSString * newString =
      [textField.text stringByReplacingCharactersInRange:range withString:string];
  BOOL const isValid = [textField.validator validateString:newString];

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
  if ([textField isEqual:self.loginTextField])
  {
    [self.passwordTextField becomeFirstResponder];
  }
  else if ([textField isEqual:self.passwordTextField])
  {
    [textField resignFirstResponder];
    [self login];
  }
  return YES;
}

- (void)storeCredentials
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  [ud setObject:self.loginTextField.text forKey:kOSMUsernameKey];
  [ud setObject:self.passwordTextField.text forKey:kOSMPasswordKey];
  [self dismissViewControllerAnimated:YES completion:nil];
}

#pragma mark - Actions

- (IBAction)login
{
  if (!self.loginButton.enabled)
    return;
  if (Platform::IsConnected())
  {
    // TODO: Add async loader spinner
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^
    {
      string username = self.loginTextField.text.UTF8String;
      string password = self.passwordTextField.text.UTF8String;
      BOOL const credentialsOK = osm::ServerApi06(username, password).CheckUserAndPassword();
      dispatch_async(dispatch_get_main_queue(), ^
      {
        if (credentialsOK)
        {
          [self storeCredentials];
        }
        else
        {
          // TODO: Add alert "wrong username or password"
        }
      });
    });
  }
  else
  {
    [self storeCredentials];
  }
}

- (IBAction)cancel
{
  [self.navigationController popViewControllerAnimated:YES];
}

#pragma mark - Properties

- (void)setIsCorrect:(MWMFieldCorrect)isCorrect
{
  _isCorrect = isCorrect;
  [self setLoginButtonEnabled:isCorrect == MWMFieldCorrectAll];
}

@end
