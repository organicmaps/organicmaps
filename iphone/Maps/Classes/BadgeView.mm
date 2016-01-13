#import "BadgeView.h"
#import "UIColor+MapsMeColor.h"
#import "UIFont+MapsMeFonts.h"

@implementation BadgeView

- (void)setValue:(NSInteger)value
{
  [self.subviews makeObjectsPerformSelector:@selector(removeFromSuperview)];
  self.hidden = value == 0;

  UIFont * font = [UIFont regular10];
  UIImage * image = [UIImage imageNamed:@"Badge"];
  CGFloat const textWidth = [@(value).stringValue sizeWithDrawSize:CGSizeMake(100, 20) font:font].width;
  CGFloat const offset = 4;
  CGFloat const imageDiameter = image.size.width;

  self.size = CGSizeMake(MAX(textWidth + 2 * offset, imageDiameter), imageDiameter);
  self.image = [image resizableImageWithCapInsets:UIEdgeInsetsMake(imageDiameter / 2, imageDiameter / 2, imageDiameter / 2, imageDiameter / 2)];

  UILabel * label = [[UILabel alloc] initWithFrame:CGRectMake(offset, 0, textWidth, self.height)];
  label.backgroundColor = [UIColor clearColor];
  label.textAlignment = NSTextAlignmentCenter;
  label.textColor = [UIColor white];
  label.font = font;
  label.text = @(value).stringValue;
  label.center = CGPointMake(self.width / 2, self.height / 2);
  [self addSubview:label];

  _value = value;
}

@end
