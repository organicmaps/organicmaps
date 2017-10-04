#import "MWMAuthorizationViewModel.h"
#import <FBSDKCoreKit/FBSDKAccessToken.h>
#import <GoogleSignIn/GoogleSignIn.h>

#include "Framework.h"

@implementation MWMAuthorizationViewModel

+ (BOOL)isAuthenticated
{
  if (GetFramework().GetUser().IsAuthenticated())
    return YES;

  auto googleToken = [GIDSignIn sharedInstance].currentUser.authentication.idToken;
  if (googleToken)
  {
    [self authenticateWithToken:googleToken type:MWMSocialTokenTypeGoogle];
    return YES;
  }

  auto fbToken = [FBSDKAccessToken currentAccessToken].tokenString;
  if (fbToken)
  {
    [self authenticateWithToken:fbToken type:MWMSocialTokenTypeFacebook];
    return YES;
  }

  return NO;
}

+ (void)authenticateWithToken:(NSString * _Nonnull)token type:(enum MWMSocialTokenType)type
{
  auto & user = GetFramework().GetUser();
  User::SocialTokenType socialTokenType;
  switch (type)
  {
  case MWMSocialTokenTypeGoogle: socialTokenType = User::SocialTokenType::Google; break;
  case MWMSocialTokenTypeFacebook: socialTokenType = User::SocialTokenType::Facebook; break;
  }
  user.Authenticate(token.UTF8String, socialTokenType);
}

@end
