#import "MWMEditorNameFooter.h"

@interface MWMEditorNameFooter ()

@property (weak, nonatomic) IBOutlet UILabel * label;

@end

@implementation MWMEditorNameFooter

+ (instancetype)footer
{
  return [[[NSBundle mainBundle] loadNibNamed:[MWMEditorNameFooter className] owner:nil options:nil]
          firstObject];
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  [self.label sizeToIntegralFit];
  self.height = self.subviews.firstObject.height;
  [super layoutSubviews];
}

- (CGFloat)height
{
  [self setNeedsLayout];
  [self layoutIfNeeded];
  return super.height;
}

@end
