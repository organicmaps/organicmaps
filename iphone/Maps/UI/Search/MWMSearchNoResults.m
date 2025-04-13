#import "MWMSearchNoResults.h"

static CGFloat const kCompactHeight = 216;
static CGFloat const kExtraCompactHeight = 52;

@interface MWMSearchNoResults ()

@property(weak, nonatomic) IBOutlet UILabel * title;
@property(weak, nonatomic) IBOutlet UILabel * text;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * textCenterY;

@end

@implementation MWMSearchNoResults

+ (instancetype)viewWithImage:(UIImage *)image title:(NSString *)title text:(NSString *)text {
  MWMSearchNoResults * view =
      [NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil].firstObject;
  if (title) {
    view.title.text = title;
  } else {
    [view.title removeFromSuperview];
  }
  view.text.text = text;
  return view;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  self.frame = self.superview.bounds;
  BOOL compact = self.height < kCompactHeight;
  self.textCenterY.priority = compact ? UILayoutPriorityDefaultHigh : UILayoutPriorityFittingSizeLevel;
  BOOL extraCompact = self.height < kExtraCompactHeight;
  self.title.hidden = extraCompact;
}

@end
