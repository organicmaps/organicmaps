
#import <Foundation/Foundation.h>
#import <FacebookSDK/FacebookSDK.h>

@interface PostMessage : NSObject

@property (nonatomic) NSString * name;
@property (nonatomic) NSString * info;
@property (nonatomic) NSString * link;

@end


@implementation PostMessage

@end


typedef NS_ENUM(NSUInteger, AccountType)
{
  AccountTypeFacebook,
  AccountTypeGooglePlus
};

@interface AccountManager : NSObject

- (void)loginToAccountType:(AccountType)type completion:(void (^)(BOOL success))block;
- (void)postMessage:(PostMessage *)message toAccountType:(AccountType)type completion:(void (^)(BOOL success))block;

+ (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation;
+ (void)applicationDidBecomeActive:(UIApplication *)application;

@end
