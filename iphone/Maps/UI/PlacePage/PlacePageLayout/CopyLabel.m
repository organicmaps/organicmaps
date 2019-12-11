#import "CopyLabel.h"

@implementation CopyLabel

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    self.userInteractionEnabled = YES;
  }
  return self;
}

- (BOOL)canBecomeFirstResponder {
  return YES;
}

- (BOOL)canPerformAction:(SEL)action withSender:(id)sender {
  return action == @selector(copy:);
}

- (void)copy:(id)sender {
  [UIPasteboard generalPasteboard].string = self.text;
}

@end
