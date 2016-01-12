#import "UIButton+Coloring.h"
#import "UIColor+MapsMeColor.h"

#import <objc/runtime.h>

namespace
{

NSString * const kDefaultPattern = @"%@_%@";
NSString * const kHighlightedPattern = @"%@_highlighted_%@";
NSString * const kSelectedPattern = @"%@_selected_%@";

} // namespace

@implementation UIButton (Coloring)

- (void)setMwm_name:(NSString *)mwm_name
{
  objc_setAssociatedObject(self, @selector(mwm_name), mwm_name, OBJC_ASSOCIATION_COPY_NONATOMIC);
  [self setDefaultImages];
}

- (NSString *)mwm_name
{
  return objc_getAssociatedObject(self, @selector(mwm_name));
}

- (void)setMwm_coloring:(MWMButtonColoring)mwm_coloring
{
  objc_setAssociatedObject(self, @selector(mwm_coloring), @(mwm_coloring), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  [self setDefaultTintColor];
}

- (void)changeColoringToOpposite
{
  if (self.mwm_coloring == MWMButtonColoringOther)
  {
    if (self.mwm_name)
    {
      [self setDefaultImages];
      self.imageView.image = [self imageForState:self.state];
    }
    return;
  }
  if (self.state == UIControlStateNormal)
    [self setDefaultTintColor];
  else if (self.state == UIControlStateHighlighted)
    [self setHighlighted:YES];
  else if (self.state == UIControlStateSelected)
    [self setSelected:YES];
}

- (void)setDefaultImages
{
  NSString * postfix = [UIColor isNightMode] ? @"dark" : @"light";
  [self setImage:[UIImage imageNamed:[NSString stringWithFormat:kDefaultPattern, self.mwm_name, postfix]] forState:UIControlStateNormal];
  [self setImage:[UIImage imageNamed:[NSString stringWithFormat:kHighlightedPattern, self.mwm_name, postfix]] forState:UIControlStateHighlighted];
  [self setImage:[UIImage imageNamed:[NSString stringWithFormat:kSelectedPattern, self.mwm_name, postfix]] forState:UIControlStateSelected];
}

- (void)setHighlighted:(BOOL)highlighted
{
  [super setHighlighted:highlighted];
  if (highlighted)
  {
    switch (self.mwm_coloring)
    {
    case MWMButtonColoringBlue:
      self.tintColor = [UIColor linkBlueDark];
      break;
    case MWMButtonColoringBlack:
      self.tintColor = [UIColor blackHintText];
      break;
    case MWMButtonColoringGray:
      self.tintColor = [UIColor blackDividers];
      break;
    case MWMButtonColoringOther:
      break;
    }
  }
  else
  {
    if (self.selected)
      return;
    [self setDefaultTintColor];
  }
  if (UIImage * image = [self imageForState:highlighted ? UIControlStateHighlighted : UIControlStateNormal])
    self.imageView.image = image;
}

- (void)setSelected:(BOOL)selected
{
  [super setSelected:selected];
  if (selected)
  {
    switch (self.mwm_coloring)
    {
    case MWMButtonColoringBlack:
      self.tintColor = [UIColor linkBlue];
      break;
    case MWMButtonColoringBlue:
    case MWMButtonColoringOther:
    case MWMButtonColoringGray:
      break;
    }
    self.imageView.image = [self imageForState:UIControlStateSelected];
  }
  else
  {
    [self setDefaultTintColor];
    self.imageView.image = [self imageForState:UIControlStateNormal];
  }
}

- (void)setDefaultTintColor
{
  switch (self.mwm_coloring)
  {
  case MWMButtonColoringBlack:
    self.tintColor = [UIColor blackSecondaryText];
    break;
  case MWMButtonColoringBlue:
    self.tintColor = [UIColor linkBlue];
    break;
  case MWMButtonColoringGray:
    self.tintColor = [UIColor blackHintText];
  case MWMButtonColoringOther:
    self.imageView.image = [self imageForState:UIControlStateNormal];
    break;
  }
}

- (MWMButtonColoring)mwm_coloring
{
  return static_cast<MWMButtonColoring>([objc_getAssociatedObject(self, @selector(mwm_coloring)) integerValue]);
}

- (void)setColoring:(NSString *)coloring
{
  if ([coloring isEqualToString:@"MWMBlue"])
    self.mwm_coloring = MWMButtonColoringBlue;
  else if ([coloring isEqualToString:@"MWMBlack"])
    self.mwm_coloring = MWMButtonColoringBlack;
  else if ([coloring isEqualToString:@"MWMOther"])
    self.mwm_coloring = MWMButtonColoringOther;
  else if ([coloring isEqualToString:@"MWMGray"])
    self.mwm_coloring = MWMButtonColoringGray;
  else
    NSAssert(false, @"Invalid UIButton's coloring!");
}

@end
