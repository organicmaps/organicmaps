#import "MWMAuthorizationViewModel.h"
#import <FBSDKCoreKit/FBSDKAccessToken.h>
#import <GoogleSignIn/GoogleSignIn.h>
#import "Statistics.h"

#include "Framework.h"

#include <memory>

@implementation MWMAuthorizationViewModel

+ (void)checkAuthenticationWithSource:(MWMAuthorizationSource)source
                           onComplete:(MWMAuthorizationCompleteBlock)onComplete
{
  if ([self isAuthenticated])
  {
    onComplete(YES);
    return;
  }

  auto googleToken = [GIDSignIn sharedInstance].currentUser.authentication.idToken;
  if (googleToken)
  {
    [self authenticateWithToken:googleToken
                           type:MWMSocialTokenTypeGoogle
                         source:source
                     onComplete:onComplete];
    return;
  }

  auto fbToken = [FBSDKAccessToken currentAccessToken].tokenString;
  if (fbToken)
  {
    [self authenticateWithToken:fbToken
                           type:MWMSocialTokenTypeFacebook
                         source:source
                     onComplete:onComplete];
    return;
  }
  onComplete(NO);
}

+ (BOOL)hasSocialToken
{
  return [GIDSignIn sharedInstance].currentUser.authentication.idToken != nil ||
         [FBSDKAccessToken currentAccessToken].tokenString != nil;
}

+ (BOOL)isAuthenticated { return GetFramework().GetUser().IsAuthenticated(); }

+ (void)authenticateWithToken:(NSString * _Nonnull)token
                         type:(MWMSocialTokenType)type
                       source:(MWMAuthorizationSource)source
                   onComplete:(MWMAuthorizationCompleteBlock)onComplete
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
  s->m_onAuthenticate = [provider, source, onComplete](bool success) {
    NSString * event = nil;
    switch (source)
    {
    case MWMAuthorizationSourceUGC:
      event = success ? kStatUGCReviewAuthRequestSuccess : kStatUGCReviewAuthError;
      break;
    case MWMAuthorizationSourceBookmarks:
      event = success ? kStatBookmarksAuthRequestSuccess : kStatBookmarksAuthRequestError;
      break;
    }

    if (success)
      [Statistics logEvent:event withParameters:@{kStatProvider: provider}];
    else
      [Statistics logEvent:event withParameters:@{kStatProvider: kStatMapsme, kStatError: @""}];
    dispatch_async(dispatch_get_main_queue(), ^{
      onComplete(success);
    });
  };
  user.AddSubscriber(std::move(s));
  user.Authenticate(token.UTF8String, socialTokenType);
}

@end
