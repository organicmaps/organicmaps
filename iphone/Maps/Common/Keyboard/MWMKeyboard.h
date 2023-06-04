#import "MWMKeyboardObserver.h"

@interface MWMKeyboard : NSObject

+ (void)applicationDidBecomeActive;

+ (void)addObserver:(id<MWMKeyboardObserver>)observer;
+ (void)removeObserver:(id<MWMKeyboardObserver>)observer;

+ (CGFloat)keyboardHeight;

- (instancetype)init __attribute__((unavailable("call +manager instead")));
- (instancetype)copy __attribute__((unavailable("call +manager instead")));
- (instancetype)copyWithZone:(NSZone *)zone __attribute__((unavailable("call +manager instead")));
+ (instancetype)allocWithZone:(struct _NSZone *)zone __attribute__((unavailable("call +manager instead")));
+ (instancetype)new __attribute__((unavailable("call +manager instead")));

@end
