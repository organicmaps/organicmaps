
#import "AccountManager.h"
#import <FacebookSDK/FacebookSDK.h>
#import "UIKitCategories.h"

@interface AccountManager ()

@property (nonatomic, copy) CompletionBlock completionBlock;

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

- (void)shareToFacebookWithCompletion:(CompletionBlock)block
{
  self.completionBlock = block;
  NSString * permissionName = @"publish_actions";
  FBSession * session = [FBSession activeSession];
  if (session.isOpen)
  {
    if ([session.permissions containsObject:permissionName])
    {
      [self shareTextToFacebookWithCompletion:block];
    }
    else
    {
      [session requestNewPublishPermissions:@[permissionName] defaultAudience:FBSessionDefaultAudienceEveryone completionHandler:^(FBSession * session, NSError * error) {
        if (error)
          block(NO);
        else
          [self shareTextToFacebookWithCompletion:block];
      }];
    }
  }
  else
  {
    [FBSession openActiveSessionWithPublishPermissions:@[permissionName] defaultAudience:FBSessionDefaultAudienceEveryone allowLoginUI:YES completionHandler:^(FBSession * session, FBSessionState state, NSError * error) {
      if (!error && state == FBSessionStateOpen)
        [self shareTextToFacebookWithCompletion:block];
      else
        block(NO);
    }];
  }
}

- (void)shareTextToFacebookWithCompletion:(CompletionBlock)block
{
  NSDictionary * parameters = @{@"message" : L(@"maps_me_is_free_today_facebook_post_ios"),
                                @"picture" : @"http://static.mapswithme.com/images/17th_august_promo.jpg",
                                @"link" : @"http://maps.me/get?17aug"};
  [FBRequestConnection startWithGraphPath:@"me/feed" parameters:parameters HTTPMethod:@"POST" completionHandler:^(FBRequestConnection * connection, id result, NSError * error) {
    block(!error);
  }];
}

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation
{
  [[FBSession activeSession] setStateChangeHandler:^(FBSession * session, FBSessionState state, NSError * error) {
    if (!error && state == FBSessionStateOpen)
      [self shareToFacebookWithCompletion:self.completionBlock];
    else
      self.completionBlock(NO);
  }];

  if ([FBAppCall handleOpenURL:url sourceApplication:sourceApplication])
    return YES;

  return NO;
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
  [FBAppCall handleDidBecomeActive];
}

@end
