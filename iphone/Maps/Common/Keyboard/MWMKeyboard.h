#import "MWMKeyboardObserver.h"

@interface MWMKeyboard : NSObject

+ (void)applicationDidBecomeActive;

+ (void)addObserver:(id<MWMKeyboardObserver>)observer;
+ (void)removeObserver:(id<MWMKeyboardObserver>)observer;

+ (CGFloat)keyboardHeight;

@end
