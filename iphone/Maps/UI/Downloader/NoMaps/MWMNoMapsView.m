#import "MWMNoMapsView.h"

@interface MWMNoMapsView ()

@property(weak, nonatomic) IBOutlet UIImageView * image;
@property(weak, nonatomic) IBOutlet UILabel * title;
@property(weak, nonatomic) IBOutlet UILabel * text;

@property(weak, nonatomic) IBOutlet NSLayoutConstraint * containerWidth;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * containerHeight;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * containerTopOffset;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * containerBottomOffset;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * imageMinHeight;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * imageHeight;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * titleImageOffset;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * titleTopOffset;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * textTopOffset;

@end

@implementation MWMNoMapsView

- (void)awakeFromNib
{
  [super awakeFromNib];
  if (!IPAD)
  {
    self.containerWidth.active = NO;
    self.containerHeight.active = NO;
  }
  else
  {
    self.containerTopOffset.active = NO;
  }
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  [self configForSize:self.frame.size];
}

- (void)configForSize:(CGSize)size
{
  CGSize iPadSize = CGSizeMake(520, 600);
  CGSize newSize = IPAD ? iPadSize : size;
  CGFloat width = newSize.width;
  CGFloat height = newSize.height;
  BOOL hideImage = (self.imageHeight.multiplier * height <= self.imageMinHeight.constant);
  if (hideImage)
  {
    self.titleImageOffset.priority = UILayoutPriorityDefaultLow;
    self.title.hidden = self.title.minY < self.titleTopOffset.constant;
    self.text.hidden = self.text.minY < self.textTopOffset.constant;
  }
  else
  {
    self.titleImageOffset.priority = UILayoutPriorityDefaultHigh;
    self.title.hidden = NO;
    self.text.hidden = NO;
  }
  self.image.hidden = hideImage;
  if (IPAD)
  {
    self.containerWidth.constant = width;
    self.containerHeight.constant = height;
  }
}
@end
