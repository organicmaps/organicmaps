@protocol MWMKeyboardObserver<NSObject>

- (void)onKeyboardAnimation;

@optional
- (void)onKeyboardWillAnimate;

@end
