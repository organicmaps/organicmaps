
#import <Foundation/Foundation.h>

@interface PostMessage : NSObject

@property (nonatomic) NSString * title;
@property (nonatomic) NSString * info;
@property (nonatomic) NSString * link;

@end


typedef NS_ENUM(NSUInteger, AccountType)
{
  AccountTypeFacebook,
  AccountTypeGooglePlus
};

typedef void (^CompletionBlock)(BOOL success);

@interface AccountManager : NSObject

- (void)loginToAccountType:(AccountType)type completion:(CompletionBlock)block;
- (void)postMessage:(PostMessage *)message toAccountType:(AccountType)type completion:(CompletionBlock)block;

+ (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation;
+ (void)applicationDidBecomeActive:(UIApplication *)application;

+ (instancetype)sharedManager;

@end
