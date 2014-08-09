
#import <Foundation/Foundation.h>

typedef void (^CompletionBlock)(BOOL success);

@interface AccountManager : NSObject

- (void)shareToFacebookWithCompletion:(CompletionBlock)block;

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation;
- (void)applicationDidBecomeActive:(UIApplication *)application;

+ (instancetype)sharedManager;

@end
