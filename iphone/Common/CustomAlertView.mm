#import "CustomAlertView.h"

@implementation CustomAlertView

- (id)initWithTitle:(NSString *)title message:(NSString *)message delegate:(id)delegate cancelButtonTitle:(NSString *)cancelButtonTitle otherButtonTitles:(NSString *)otherButtonTitles, ...
{
  if ((self = [super initWithTitle:title message:message delegate:delegate cancelButtonTitle:cancelButtonTitle otherButtonTitles:nil, nil]))
  {
    va_list args;
    va_start(args, otherButtonTitles);
    for (NSString * anOtherButtonTitle = otherButtonTitles; anOtherButtonTitle != nil; anOtherButtonTitle = va_arg(args, NSString*))
      [self addButtonWithTitle:anOtherButtonTitle];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationDidEnterBackground:) name:UIApplicationDidEnterBackgroundNotification object:nil];
  }
  return self;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)applicationDidEnterBackground:(id) sender
{
  // We should not be here when entering back to foreground state
  [self dismissWithClickedButtonIndex:[self cancelButtonIndex] animated:NO];
}

@end