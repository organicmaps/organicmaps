#import "MWMPlaceDoesntExistAlert.h"

@interface MWMPlaceDoesntExistAlert()

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

@end
