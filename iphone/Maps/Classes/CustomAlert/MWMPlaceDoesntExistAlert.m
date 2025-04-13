#import "MWMPlaceDoesntExistAlert.h"
#import "MWMKeyboard.h"

@interface MWMPlaceDoesntExistAlert ()<MWMKeyboardObserver>

@property(weak, nonatomic) IBOutlet UITextField * textField;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * centerHorizontaly;
@property(copy, nonatomic) MWMStringBlock block;

@end

@implementation MWMPlaceDoesntExistAlert

+ (instancetype)alertWithBlock:(MWMStringBlock)block
{
  MWMPlaceDoesntExistAlert * alert =
      [NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil].firstObject;
  alert.block = block;
  [MWMKeyboard addObserver:alert];
  return alert;
}

- (IBAction)rightButtonTap
{
  [self.textField resignFirstResponder];
  [self close:^{
    self.block(self.textField.text);
  }];
}

- (IBAction)leftButtonTap
{
  [self.textField resignFirstResponder];
  [self close:nil];
}

#pragma mark - MWMKeyboard

- (void)onKeyboardAnimation
{
  self.centerHorizontaly.constant = -[MWMKeyboard keyboardHeight] / 2;
  [self layoutIfNeeded];
}

- (void)onKeyboardWillAnimate { [self setNeedsLayout]; }
@end
