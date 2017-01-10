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
  [super drawTextInRect:UIEdgeInsetsInsetRect(rect, {.left = 16})];
}

@end
