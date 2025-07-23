#import "MetalView.h"

@implementation MetalView

// The Metal view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id)initWithCoder:(NSCoder *)coder
{
  NSLog(@"MetalView initWithCoder Started");
  self = [super initWithCoder:coder];
  if (self)
    [self initialize];

  NSLog(@"MetalView initWithCoder Ended");
  return self;
}

- (void)initialize
{
  self.device = MTLCreateSystemDefaultDevice();
  if (!self.device)
  {
    self.opaque = NO;
    self.hidden = YES;
    NSLog(@"Metal is not supported on this device");
    return;
  }
  self.opaque = YES;
  self.hidden = NO;
  self.paused = TRUE;
  self.enableSetNeedsDisplay = FALSE;
  self.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 0.0);
  self.contentScaleFactor = [[UIScreen mainScreen] nativeScale];
}

@end
