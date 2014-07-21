
#import "AccountManager.h"
#import <FacebookSDK/FacebookSDK.h>

@implementation PostMessage

@end


@implementation AccountManager

+ (instancetype)sharedManager
{
  static AccountManager * manager;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    manager = [[self alloc] init];
  });
  return manager;
}

#pragma mark - Posting

- (void)postMessage:(PostMessage *)message toAccountType:(AccountType)type completion:(CompletionBlock)block;
{
  switch (type)
  {
    case AccountTypeFacebook:
    {
      [self postMessage:message toFacebookAndCheckPermissionsWithCompletion:block];
      break;
    }
    case AccountTypeGooglePlus:
    {
      break;
    }
  }
}


- (void)postMessage:(PostMessage *)message toFacebookAndCheckPermissionsWithCompletion:(CompletionBlock)block
{
  if ([self hasPublishPermissionsForFacebook])
  {
    [self postMessage:message toFacebookWithCompletion:^(BOOL success) {
      block(success);
    }];
  }
  else
  {
    [[FBSession activeSession] requestNewPublishPermissions:@[@"publish_actions"] defaultAudience:FBSessionDefaultAudienceFriends completionHandler:^(FBSession * session, NSError * error) {
      if (!error && [self hasPublishPermissionsForFacebook])
      {
        [self postMessage:message toFacebookWithCompletion:^(BOOL success) {
          block(success);
        }];
      }
      else
      {
        NSLog(@"Unable to get publish permissions for Facebook");
        block(NO);
      }
    }];
  }
}

- (void)postMessage:(PostMessage *)message toFacebookWithCompletion:(CompletionBlock)block
{
  FBLinkShareParams * parameters = [[FBLinkShareParams alloc] init];
  parameters.name = message.title;
  parameters.link = [NSURL URLWithString:message.link];

  if ([FBDialogs canPresentShareDialogWithParams:parameters])
  {
    [FBDialogs presentShareDialogWithParams:parameters clientState:nil handler:^(FBAppCall * call, NSDictionary * results, NSError * error) {
      if (error)
        NSLog(@"Unable to post message on the wall of Facebook (%@)", error);
      block(!error);
    }];
  }
  else
  {
    NSMutableDictionary * parameters = [[NSMutableDictionary alloc] init];
    if (message.title)
      parameters[@"name"] = message.title;
    if (message.link)
      parameters[@"link"] = message.link;
    if (message.info)
      parameters[@"description"] = message.info;

    [FBWebDialogs presentFeedDialogModallyWithSession:nil parameters:parameters handler:^(FBWebDialogResult result, NSURL * resultURL, NSError * error) {
      if (error || result == FBWebDialogResultDialogCompleted)
        NSLog(@"Unable to post message on the wall of Facebook (%@)", error);
      block(!error && result != FBWebDialogResultDialogNotCompleted);
    }];
  }
}

- (BOOL)hasPublishPermissionsForFacebook
{
  return [[FBSession activeSession].permissions containsObject:@"publish_actions"];
}

#pragma mark - Logining

+ (void)sendUserInfo:(NSDictionary *)userInfo ofAccount:(AccountType)type toZOGServerWithCompletion:(CompletionBlock)block
{
  //TODO: implement
  block(YES);
}

+ (void)getFacebookUserInfoAndId:(CompletionBlock)block
{
  [[FBRequest requestForMe] startWithCompletionHandler:^(FBRequestConnection * connection, NSDictionary<FBGraphUser> * user, NSError * error) {
    if (error)
    {
      NSLog(@"Unable to get user info and id of Facebook (%@)", error);
      block(NO);
    }
    else
    {
      NSMutableDictionary * userInfo = [[NSMutableDictionary alloc] init];
      if (user.objectID)
        userInfo[@"Id"] = user.objectID;
      if (user.first_name)
        userInfo[@"FirstName"] = user.first_name;
      if (user.middle_name)
        userInfo[@"MiddleName"] = user.middle_name;
      if (user.last_name)
        userInfo[@"LastName"] = user.last_name;
      if (user.link)
        userInfo[@"FacebookLink"] = user.link;
      if (user.username)
        userInfo[@"FacebookUserName"] = user.username;
      if (user[@"birthday"])
        userInfo[@"Birthday"] = user[@"birthday"];
      if (user[@"email"])
        userInfo[@"Email"] = user[@"email"];

      [self sendUserInfo:userInfo ofAccount:AccountTypeFacebook toZOGServerWithCompletion:^(BOOL success) {
        block(success);
      }];
    }
  }];
}

- (void)loginToAccountType:(AccountType)type completion:(CompletionBlock)block
{
  switch (type)
  {
    case AccountTypeFacebook:
    {
      if ([FBSession activeSession].state == FBSessionStateOpen || [FBSession activeSession].state == FBSessionStateOpenTokenExtended)
      {
        block(YES);
      }
      else
      {
        [FBSession openActiveSessionWithReadPermissions:@[@"public_profile"] allowLoginUI:YES completionHandler:^(FBSession *session, FBSessionState state, NSError *error) {
          if (!error && state == FBSessionStateOpen)
          {
            [[self class] getFacebookUserInfoAndId:block];
          }
          else
          {
            NSLog(@"Unable to open Facebook session (%@)", error);
            block(NO);
          }
        }];
      }
      break;
    }
    case AccountTypeGooglePlus:
    {
      break;
    }
  }
}

+ (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation
{
  [[FBSession activeSession] setStateChangeHandler:^(FBSession * session, FBSessionState state, NSError * error) {
    if (!error && state == FBSessionStateOpen)
      [self getFacebookUserInfoAndId:^(BOOL success){}];
    else
      NSLog(@"Unable to open Facebook session (%@)", error);
//    [self sessionStateChanged:session state:state error:error];
  }];

  if ([FBAppCall handleOpenURL:url sourceApplication:sourceApplication])
    return YES;

  return NO;
}

+ (void)applicationDidBecomeActive:(UIApplication *)application
{
  [FBAppCall handleDidBecomeActive];
}


// FACEBOOK ERROR HANDLING LOGIC. KEEP FOR TESTING

// This method will handle ALL the session state changes in the app
//+ (void)sessionStateChanged:(FBSession *)session state:(FBSessionState)state error:(NSError *)error
//{
//  // If the session was opened successfully
//  if (!error && state == FBSessionStateOpen)
//  {
//    NSLog(@"Session opened");
//    // Show the user the logged-in UI
//    return;
//  }
//  if (state == FBSessionStateClosed || state == FBSessionStateClosedLoginFailed)
//  {
//    // If the session is closed
//    NSLog(@"Session closed");
//    // Show the user the logged-out UI
//  }
//
//  // Handle errors
//  if (error)
//  {
//    NSLog(@"Error");
//    NSString *alertText;
//    NSString *alertTitle;
//    // If the error requires people using an app to make an action outside of the app in order to recover
//    if ([FBErrorUtility shouldNotifyUserForError:error] == YES)
//    {
//      alertTitle = @"Something went wrong";
//      alertText = [FBErrorUtility userMessageForError:error];
////      [self showMessage:alertText withTitle:alertTitle];
//    }
//    else
//    {
//
//      // If the user cancelled login, do nothing
//      if ([FBErrorUtility errorCategoryForError:error] == FBErrorCategoryUserCancelled)
//      {
//        NSLog(@"User cancelled login");
//
//        // Handle session closures that happen outside of the app
//      }
//      else if ([FBErrorUtility errorCategoryForError:error] == FBErrorCategoryAuthenticationReopenSession)
//      {
//        alertTitle = @"Session Error";
//        alertText = @"Your current session is no longer valid. Please log in again.";
////        [self showMessage:alertText withTitle:alertTitle];
//
//        // Here we will handle all other errors with a generic error message.
//        // We recommend you check our Handling Errors guide for more information
//        // https://developers.facebook.com/docs/ios/errors/
//      }
//      else
//      {
//        //Get more error information from the error
//        NSDictionary * errorInformation = [[[error.userInfo objectForKey:@"com.facebook.sdk:ParsedJSONResponseKey"] objectForKey:@"body"] objectForKey:@"error"];
//
//        // Show the user an error message
//        alertTitle = @"Something went wrong";
//        alertText = [NSString stringWithFormat:@"Please retry. \n\n If the problem persists contact us and mention this error code: %@", [errorInformation objectForKey:@"message"]];
////        [self showMessage:alertText withTitle:alertTitle];
//      }
//    }
//    // Clear this token
//    [FBSession.activeSession closeAndClearTokenInformation];
//    // Show the user the logged-out UI
////    [self userLoggedOut];
//  }
//}

@end
