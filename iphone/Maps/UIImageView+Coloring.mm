#import "UIImageView+Coloring.h"
#import "UIColor+MapsMeColor.h"

#import <objc/runtime.h>

@implementation UIImageView (Coloring)

- (void)setMwm_name:(NSString *)mwm_name
{
  objc_setAssociatedObject(self, @selector(mwm_name), mwm_name, OBJC_ASSOCIATION_COPY_NONATOMIC);
  self.image = [UIImage imageNamed:[NSString stringWithFormat:@"%@_%@", mwm_name, [UIColor isNightMode] ? @"dark" : @"light"]];
}

- (NSString *)mwm_name
{
  return objc_getAssociatedObject(self, @selector(mwm_name));
}

- (void)setMwm_coloring:(MWMImageColoring)mwm_coloring
{
  objc_setAssociatedObject(self, @selector(mwm_coloring), @(mwm_coloring), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  if (mwm_coloring != MWMImageColoringOther)
    self.tintColor = [[UIColor class] performSelector:self.coloringSelector];
#pragma clang diagnostic pop
}

- (MWMImageColoring)mwm_coloring
{
  return static_cast<MWMImageColoring>([objc_getAssociatedObject(self, @selector(mwm_coloring)) integerValue]);
}

- (void)changeColoringToOpposite
{
  if (self.mwm_coloring == MWMImageColoringOther)
  {
    if (self.mwm_name)
      self.image = [UIImage imageNamed:[NSString stringWithFormat:@"%@_%@", self.mwm_name, [UIColor isNightMode] ? @"dark" : @"light"]];
    return;
  }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  self.tintColor = [[UIColor class] performSelector:self.coloringSelector];
#pragma clang diagnostic pop
}

- (SEL)coloringSelector
{
  switch (self.mwm_coloring)
  {
  case MWMImageColoringBlack:
    return @selector(blackSecondaryText);
  case MWMImageColoringBlue:
    return @selector(linkBlue);
  case MWMImageColoringGray:
    return @selector(blackHintText);
  case MWMImageColoringOther:
    return @selector(white);
  }
}

- (void)setHighlighted:(BOOL)highlighted
{
  switch (self.mwm_coloring)
  {
  case MWMImageColoringBlack:
    self.tintColor = highlighted ? [UIColor linkBlue] : [UIColor blackSecondaryText];
    break;

  case MWMImageColoringGray:
    self.tintColor = highlighted ? [UIColor blackSecondaryText] : [UIColor blackHintText];
    break;
  case MWMImageColoringBlue:
  case MWMImageColoringOther:
    break;
  }
}

- (void)setColoring:(NSString *)coloring
{
  if ([coloring isEqualToString:@"MWMBlue"])
    self.mwm_coloring = MWMImageColoringBlue;
  else if ([coloring isEqualToString:@"MWMBlack"])
    self.mwm_coloring = MWMImageColoringBlack;
  else if ([coloring isEqualToString:@"MWMOther"])
    self.mwm_coloring = MWMImageColoringOther;
  else if ([coloring isEqualToString:@"MWMGray"])
    self.mwm_coloring = MWMImageColoringGray;
  else
    NSAssert(false, @"Incorrect UIImageView's coloring");
}

@end
