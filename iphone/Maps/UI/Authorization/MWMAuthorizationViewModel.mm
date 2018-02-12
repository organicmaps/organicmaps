#import "MWMAuthorizationViewModel.h"
#import <FBSDKCoreKit/FBSDKAccessToken.h>
#import <GoogleSignIn/GoogleSignIn.h>
#import "Statistics.h"

#include "Framework.h"

#include <memory>

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
  NSString * provider = nil;
  switch (type)
  {
  case MWMSocialTokenTypeGoogle:
    provider = kStatGoogle;
    socialTokenType = User::SocialTokenType::Google;
    break;
  case MWMSocialTokenTypeFacebook:
    provider = kStatFacebook;
    socialTokenType = User::SocialTokenType::Facebook;
    break;
  }
  
  auto s = std::make_unique<User::Subscriber>();
  s->m_postCallAction = User::Subscriber::Action::RemoveSubscriber;
  s->m_onAuthenticate = [provider](bool success) {
    if (success)
      [Statistics logEvent:kStatUGCReviewAuthRequestSuccess
            withParameters:@{kStatProvider: provider}];
    else
      [Statistics logEvent:kStatUGCReviewAuthError
            withParameters:@{kStatProvider: kStatMapsme, kStatError: @""}];
  };
  user.AddSubscriber(std::move(s));
  user.Authenticate(token.UTF8String, socialTokenType);
}

@end
