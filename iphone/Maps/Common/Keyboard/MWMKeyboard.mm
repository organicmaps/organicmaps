#import "MWMKeyboard.h"

namespace
{
using TObserver = id<MWMKeyboardObserver>;
using TObservers = NSHashTable<__kindof TObserver>;
}  // namespace

@interface MWMKeyboard ()

@property(nonatomic) TObservers * observers;
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
    _observers = [TObservers weakObjectsHashTable];
    NSNotificationCenter * nc = [NSNotificationCenter defaultCenter];
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

- (void)dealloc { [[NSNotificationCenter defaultCenter] removeObserver:self]; }
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

- (void)keyboardWillShow:(NSNotification *)notification
{
  for (TObserver observer in self.observers)
  {
    if ([observer respondsToSelector:@selector(onKeyboardWillAnimate)])
      [observer onKeyboardWillAnimate];
  }
  CGSize const keyboardSize =
      [notification.userInfo[UIKeyboardFrameBeginUserInfoKey] CGRectValue].size;
  self.keyboardHeight = MIN(keyboardSize.height, keyboardSize.width);
  NSNumber * rate = notification.userInfo[UIKeyboardAnimationDurationUserInfoKey];
  [UIView animateWithDuration:rate.floatValue
                   animations:^{
                     for (TObserver observer in self.observers)
                       [observer onKeyboardAnimation];
                   }];
}

- (void)keyboardWillHide:(NSNotification *)notification
{
  for (TObserver observer in self.observers)
  {
    if ([observer respondsToSelector:@selector(onKeyboardWillAnimate)])
      [observer onKeyboardWillAnimate];
  }
  self.keyboardHeight = 0;
  NSNumber * rate = notification.userInfo[UIKeyboardAnimationDurationUserInfoKey];
  [UIView animateWithDuration:rate.floatValue
                   animations:^{
                     for (TObserver observer in self.observers)
                       [observer onKeyboardAnimation];
                   }];
}

@end
