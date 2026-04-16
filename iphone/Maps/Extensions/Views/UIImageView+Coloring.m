#import "UIImageView+Coloring.h"

#import <objc/runtime.h>

@implementation UIImageView (Coloring)

- (void)setMwm_name:(NSString *)mwm_name
{
  objc_setAssociatedObject(self, @selector(mwm_name), mwm_name, OBJC_ASSOCIATION_COPY_NONATOMIC);
  self.image = [UIImage imageNamed:mwm_name];
}

- (NSString *)mwm_name
{
  return objc_getAssociatedObject(self, @selector(mwm_name));
}
- (void)setMwm_coloring:(MWMImageColoring)mwm_coloring
{
  objc_setAssociatedObject(self, @selector(mwm_coloring), @(mwm_coloring), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  if (mwm_coloring == MWMImageColoringOther)
    return;
  [self applyColoring];
}

- (MWMImageColoring)mwm_coloring
{
  return [objc_getAssociatedObject(self, @selector(mwm_coloring)) integerValue];
}

- (void)applyColoring
{
  switch (self.mwm_coloring)
  {
  case MWMImageColoringWhite: self.tintColor = [UIColor whitePrimary]; break;
  case MWMImageColoringBlack: self.tintColor = [UIColor blackSecondaryText]; break;
  case MWMImageColoringBlue: self.tintColor = [UIColor linkBlue]; break;
  case MWMImageColoringGray: self.tintColor = [UIColor blackHintText]; break;
  case MWMImageColoringOther: self.tintColor = [UIColor whitePrimary]; break;
  case MWMImageColoringSeparator: self.tintColor = [UIColor blackDividers]; break;
  }
}

- (void)changeColoringToOpposite
{
  if (self.mwm_coloring == MWMImageColoringOther)
  {
    if (self.mwm_name)
      self.image = [UIImage imageNamed:self.mwm_name];
    return;
  }
  [self applyColoring];
}

- (void)setHighlighted:(BOOL)highlighted
{
  switch (self.mwm_coloring)
  {
  case MWMImageColoringWhite: self.tintColor = highlighted ? [UIColor whiteHintText] : [UIColor whitePrimary]; break;
  case MWMImageColoringBlack:
    self.tintColor = highlighted ? [UIColor blackHintText] : [UIColor blackSecondaryText];
    break;
  case MWMImageColoringGray:
    self.tintColor = highlighted ? [UIColor blackSecondaryText] : [UIColor blackHintText];
    break;
  case MWMImageColoringOther:
  case MWMImageColoringBlue:
  case MWMImageColoringSeparator: break;
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
  else if ([coloring isEqualToString:@"MWMSeparator"])
    self.mwm_coloring = MWMImageColoringSeparator;
  else if ([coloring isEqualToString:@"MWMWhite"])
    self.mwm_coloring = MWMImageColoringWhite;
  else
    NSAssert(false, @"Incorrect UIImageView's coloring");
}

@end
