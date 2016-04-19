#import "MWMPlaceDoesntExistAlert.h"

// This private class needs for change default text field's content inset.
@interface _MWMTextField : UITextField

@end

@implementation _MWMTextField

// placeholder position
- (CGRect)textRectForBounds:(CGRect)bounds
{
  return CGRectInset(bounds, 4, 4);
}

// text position
- (CGRect)editingRectForBounds:(CGRect)bounds
{
  return CGRectInset(bounds, 4, 4);
}

@end

@interface MWMPlaceDoesntExistAlert ()

@property (weak, nonatomic) IBOutlet UITextField * textField;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * centerHorizontaly;
@property (copy, nonatomic) MWMStringBlock block;

@end

@implementation MWMPlaceDoesntExistAlert

+ (instancetype)alertWithBlock:(MWMStringBlock)block
{
  MWMPlaceDoesntExistAlert * alert = [[[NSBundle mainBundle] loadNibNamed:[MWMPlaceDoesntExistAlert className] owner:nil
                                                                  options:nil] firstObject];
  alert.block = block;

  [[NSNotificationCenter defaultCenter] addObserver:alert
                                           selector:@selector(keyboardWillShow:)
                                               name:UIKeyboardWillShowNotification
                                             object:nil];

  [[NSNotificationCenter defaultCenter] addObserver:alert
                                           selector:@selector(keyboardWillHide:)
                                               name:UIKeyboardWillHideNotification
                                             object:nil];
  return alert;
}

- (IBAction)rightButtonTap
{
  [self.textField resignFirstResponder];
  [self close];
  self.block(self.textField.text);
}

- (IBAction)leftButtonTap
{
  [self.textField resignFirstResponder];
  [self close];
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)keyboardWillShow:(NSNotification *)notification
{
  CGFloat const keyboardHeight = [notification.userInfo[UIKeyboardFrameBeginUserInfoKey] CGRectValue].size.height;
  NSNumber * rate = notification.userInfo[UIKeyboardAnimationDurationUserInfoKey];
  [self setNeedsLayout];
  self.centerHorizontaly.constant = - keyboardHeight / 2;
  [UIView animateWithDuration:rate.floatValue animations:^
  {
    [self layoutIfNeeded];
  }];
}

- (void)keyboardWillHide:(NSNotification *)notification
{
  NSNumber * rate = notification.userInfo[UIKeyboardAnimationDurationUserInfoKey];
  [self setNeedsLayout];
  self.centerHorizontaly.constant = 0;
  [UIView animateWithDuration:rate.floatValue animations:^
  {
    [self layoutIfNeeded];
  }];
}

@end
