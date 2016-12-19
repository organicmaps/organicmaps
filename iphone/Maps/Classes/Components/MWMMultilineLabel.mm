#import "MWMCommon.h"
#import "MWMMultilineLabel.h"

@implementation MWMMultilineLabel

- (void)setBounds:(CGRect)bounds
{
  [super setBounds:bounds];

  // If this is a multiline label, need to make sure
  // preferredMaxLayoutWidth always matches the frame width
  // (i.e. orientation change can mess this up)
  if (self.numberOfLines == 0 && !equalScreenDimensions(bounds.size.width, self.preferredMaxLayoutWidth))
  {
    self.preferredMaxLayoutWidth = self.bounds.size.width;
    [self setNeedsUpdateConstraints];
  }
}

@end
