#import "MWMMapDownloaderCellHeader.h"

@implementation MWMMapDownloaderCellHeader

- (instancetype)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];
  if (self)
  {
    self.font = [UIFont regular12];
    self.textColor = [UIColor blackSecondaryText];
  }
  return self;
}

- (void)drawTextInRect:(CGRect)rect
{
  rect = UIEdgeInsetsInsetRect(rect, {.left = 16});
  if (@available(iOS 11.0, *))
    rect = UIEdgeInsetsInsetRect(rect, self.safeAreaInsets);
  [super drawTextInRect:rect];
}

@end
