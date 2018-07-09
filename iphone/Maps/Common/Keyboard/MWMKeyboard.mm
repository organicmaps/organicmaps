#import "MWMKeyboard.h"

namespace
{
using Observer = id<MWMKeyboardObserver>;
using Observers = NSHashTable<Observer>;
}  // namespace

@interface MWMKeyboard ()

@property(nonatomic) Observers * observers;
@property(nonatomic) CGFloat keyboardHeight;

@end

@implementation MWMKeyboard

+ (void)applicationDidBecomeActive { [self manager]; }
+ (MWMKeyboard *)manager
{
  static MWMKeyboard * manager;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    manager = [[super alloc] initManager];
  });
  return manager;
}

- (instancetype)initManager
{
  self = [super init];
  if (self)
  {
    _observers = [Observers weakObjectsHashTable];
    NSNotificationCenter * nc = NSNotificationCenter.defaultCenter;
    [nc addObserver:self
           selector:@selector(keyboardWillShow:)
               name:UIKeyboardWillShowNotification
             object:nil];

    [nc addObserver:self
           selector:@selector(keyboardWillHide:)
               name:UIKeyboardWillHideNotification
             object:nil];
  }
  return self;
}

- (void)dealloc { [NSNotificationCenter.defaultCenter removeObserver:self]; }
+ (CGFloat)keyboardHeight { return [self manager].keyboardHeight; }
#pragma mark - Add/Remove Observers

+ (void)addObserver:(id<MWMKeyboardObserver>)observer
{
  [[self manager].observers addObject:observer];
}

+ (void)removeObserver:(id<MWMKeyboardObserver>)observer
{
  [[self manager].observers removeObject:observer];
}

#pragma mark - Notifications

- (void)onKeyboardWillAnimate
{
  Observers * observers = self.observers.copy;
  for (Observer observer in observers)
  {
    if ([observer respondsToSelector:@selector(onKeyboardWillAnimate)])
      [observer onKeyboardWillAnimate];
  }
}

- (void)onKeyboardAnimation
{
  Observers * observers = self.observers.copy;
  for (Observer observer in observers)
    [observer onKeyboardAnimation];
}

- (void)keyboardWillShow:(NSNotification *)notification
{
  [self onKeyboardWillAnimate];
  CGSize const keyboardSize =
      [notification.userInfo[UIKeyboardFrameBeginUserInfoKey] CGRectValue].size;
  self.keyboardHeight = MIN(keyboardSize.height, keyboardSize.width);
  NSNumber * rate = notification.userInfo[UIKeyboardAnimationDurationUserInfoKey];
  [UIView animateWithDuration:rate.floatValue
                   animations:^{
                     [self onKeyboardAnimation];
                   }];
}

- (void)keyboardWillHide:(NSNotification *)notification
{
  [self onKeyboardWillAnimate];
  self.keyboardHeight = 0;
  NSNumber * rate = notification.userInfo[UIKeyboardAnimationDurationUserInfoKey];
  [UIView animateWithDuration:rate.floatValue
                   animations:^{
                     [self onKeyboardAnimation];
                   }];
}

@end
