#import "MWMMapDownloaderCellHeader.h"
#import "UIColor+MapsMeColor.h"
#import "UIFont+MapsMeFonts.h"

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
