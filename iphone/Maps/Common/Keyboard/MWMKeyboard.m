#import "MWMKeyboard.h"

@interface MWMKeyboard ()

@property(nonatomic) NSHashTable *observers;
@property(nonatomic) CGFloat keyboardHeight;

@end

@implementation MWMKeyboard

+ (void)applicationDidBecomeActive {
  [self manager];
}

+ (MWMKeyboard *)manager {
  static MWMKeyboard *manager;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    manager = [[self alloc] initManager];
  });
  return manager;
}

- (instancetype)initManager {
  self = [super init];
  if (self) {
    _observers = [NSHashTable weakObjectsHashTable];
    NSNotificationCenter *nc = NSNotificationCenter.defaultCenter;
    [nc addObserver:self selector:@selector(keyboardWillShow:) name:UIKeyboardWillShowNotification object:nil];
    [nc addObserver:self selector:@selector(keyboardWillHide:) name:UIKeyboardWillHideNotification object:nil];
  }
  return self;
}

- (void)dealloc {
  [NSNotificationCenter.defaultCenter removeObserver:self];
}

+ (CGFloat)keyboardHeight {
  return [self manager].keyboardHeight;
}

#pragma mark - Add/Remove Observers

+ (void)addObserver:(id<MWMKeyboardObserver>)observer {
  [[self manager].observers addObject:observer];
}

+ (void)removeObserver:(id<MWMKeyboardObserver>)observer {
  [[self manager].observers removeObject:observer];
}

#pragma mark - Notifications

- (void)onKeyboardWillAnimate {
  for (id<MWMKeyboardObserver> observer in self.observers) {
    if ([observer respondsToSelector:@selector(onKeyboardWillAnimate)])
      [observer onKeyboardWillAnimate];
  }
}

- (void)onKeyboardAnimation {
  for (id<MWMKeyboardObserver> observer in self.observers) {
    [observer onKeyboardAnimation];
  }
}

- (void)keyboardWillShow:(NSNotification *)notification {
  [self onKeyboardWillAnimate];
  CGSize keyboardSize = [notification.userInfo[UIKeyboardFrameBeginUserInfoKey] CGRectValue].size;
  self.keyboardHeight = MIN(keyboardSize.height, keyboardSize.width);
  NSNumber *duration = notification.userInfo[UIKeyboardAnimationDurationUserInfoKey];
  NSNumber *curve = notification.userInfo[UIKeyboardAnimationCurveUserInfoKey];
  [UIView animateWithDuration:duration.doubleValue
                        delay:0
                      options:curve.integerValue
                   animations:^{
                     [self onKeyboardAnimation];
                   }
                   completion:nil];
}

- (void)keyboardWillHide:(NSNotification *)notification {
  [self onKeyboardWillAnimate];
  self.keyboardHeight = 0;
  NSNumber *duration = notification.userInfo[UIKeyboardAnimationDurationUserInfoKey];
  NSNumber *curve = notification.userInfo[UIKeyboardAnimationCurveUserInfoKey];
  [UIView animateWithDuration:duration.doubleValue
                        delay:0
                      options:curve.integerValue
                   animations:^{
                     [self onKeyboardAnimation];
                   }
                   completion:nil];
}

@end
