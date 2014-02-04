
#import <Foundation/Foundation.h>

extern NSString * const InAppMessageInterstitial;
extern NSString * const InAppMessageBanner;

@interface InAppMessagesManager : NSObject

@property (nonatomic, weak) UIViewController * currentController;

- (void)registerController:(UIViewController *)vc forMessage:(NSString *)messageName;
- (void)unregisterControllerFromAllMessages:(UIViewController *)vc;

- (void)triggerMessage:(NSString *)messageName;

+ (instancetype)sharedManager;

@end
