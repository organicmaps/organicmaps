#import "MWMNoMapsView.h"

@interface MWMNoMapsView ()

@property (weak, nonatomic) IBOutlet UIImageView * image;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * containerWidth;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * containerHeight;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * imageMinHeight;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * imageHeight;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleImageOffset;

@end

@implementation MWMNoMapsView

- (void)setFrame:(CGRect)frame
{
  super.frame = frame;
  [self configForSize:self.frame.size];
}

- (void)setBounds:(CGRect)bounds
{
  super.bounds = bounds;
  [self configForSize:self.bounds.size];
}

- (void)configForSize:(CGSize)size
{
  CGSize const iPadSize = {520, 534};
  CGSize const newSize = IPAD ? iPadSize : size;
  CGFloat const width = newSize.width;
  CGFloat const height = newSize.height;
  BOOL const hideImage = (self.imageHeight.multiplier * height <= self.imageMinHeight.constant);
  self.titleImageOffset.priority = hideImage ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;
  self.image.hidden = hideImage;
  self.containerWidth.constant = width;
  self.containerHeight.constant = height;
}

@end
