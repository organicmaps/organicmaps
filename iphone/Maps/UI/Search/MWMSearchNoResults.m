#import "MWMSearchNoResults.h"

static CGFloat const kCompactHeight = 216;
static CGFloat const kExtraCompactHeight = 52;

@interface MWMSearchNoResults ()

@property(weak, nonatomic) IBOutlet UIImageView * image;
@property(weak, nonatomic) IBOutlet UILabel * title;
@property(weak, nonatomic) IBOutlet UILabel * text;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * textCenterY;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * textOffsetToImage;

@end

@implementation MWMSearchNoResults

+ (instancetype)viewWithImage:(UIImage *)image title:(NSString *)title text:(NSString *)text {
  MWMSearchNoResults * view =
      [NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil].firstObject;
  view.image.image = image;
  if (title) {
    view.title.text = title;
  } else {
    [view.title removeFromSuperview];
    view.textOffsetToImage.priority = UILayoutPriorityDefaultHigh;
  }
  view.text.text = text;
  return view;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  self.frame = self.superview.bounds;
  BOOL compact = self.height < kCompactHeight;
  self.image.hidden = compact;
  self.textCenterY.priority = compact ? UILayoutPriorityDefaultHigh : UILayoutPriorityFittingSizeLevel;
  BOOL extraCompact = self.height < kExtraCompactHeight;
  self.title.hidden = extraCompact;
}

@end
