#import "MWMDirectionView.h"
#import "MWMPlacePageViewManager.h"
#import "UIFont+MapsMeFonts.h"

static NSString * const kDirectionViewNibName = @"MWMDirectionView";
static CGFloat const kDirectionArrowSide = IPAD ? 260. : 160.;

@interface MWMDirectionView ()

@property (weak, nonatomic) MWMPlacePageViewManager * manager;
@property (nonatomic) CGSize defaultSize;

@end

@implementation MWMDirectionView

- (instancetype)initWithManager:(MWMPlacePageViewManager *)manager
{
  self = [[[NSBundle mainBundle] loadNibNamed:kDirectionViewNibName owner:nil options:nil] firstObject];
  [self setup:manager];
  return self;
}

- (void)setup:(MWMPlacePageViewManager *)manager
{
  self.manager = manager;
  self.directionArrow.size = CGSizeMake(kDirectionArrowSide, kDirectionArrowSide);
  self.directionArrow.image = [UIImage imageNamed:IPAD ? @"direction_big" : @"direction_mini"];

  NSString * const kFontName = @"HelveticaNeue";
  self.titleLabel.font = self.distanceLabel.font = IPAD ? [UIFont fontWithName:kFontName size:52.] : [UIFont fontWithName:kFontName size:32.];
  self.typeLabel.font = IPAD ? [UIFont regular24] : [UIFont regular16];

  self.autoresizingMask = self.contentView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
  self.directionArrow.autoresizingMask = UIViewAutoresizingNone;
}

- (void)layoutSubviews
{
  UIView * superview = self.superview;
  self.frame = superview.bounds;
  CGSize const size = self.superview.size;
  self.size = size;
  CGFloat const minimumBorderOffset = 40.;
  BOOL const isLandscape = size.width > size.height;
  [superview bringSubviewToFront:self];
  if (isLandscape)
  {
    CGFloat const defaultWidth = size.width - 3. * minimumBorderOffset - kDirectionArrowSide;
    [self resizeLabelsWithWidth:defaultWidth];
    CGFloat const titleOffset = 8.;
    CGFloat const typeOffset = 24.;
    CGFloat const contentViewHeight = size.height - 2. * minimumBorderOffset;
    CGFloat const contentViewOffset = (size.width - self.titleLabel.width - minimumBorderOffset - self.directionArrow.width) / 2.;
    CGFloat const contentViewWidth = self.titleLabel.width + minimumBorderOffset + self.directionArrow.width;
    self.contentView.frame = CGRectMake(contentViewOffset, minimumBorderOffset, contentViewWidth, contentViewHeight);
    self.directionArrow.center = CGPointMake(kDirectionArrowSide / 2., self.contentView.height / 2.);
    CGFloat const directionArrowOffsetX = self.directionArrow.maxX + minimumBorderOffset;
    CGFloat const actualLabelsBlockHeight = self.titleLabel.height + titleOffset + self.typeLabel.height + typeOffset + self.distanceLabel.height;
    CGFloat const labelsBlockTopOffset = (contentViewHeight - actualLabelsBlockHeight) / 2.;
    self.titleLabel.origin = CGPointMake(directionArrowOffsetX, labelsBlockTopOffset);
    self.titleLabel.textAlignment = NSTextAlignmentLeft;
    self.typeLabel.origin = CGPointMake(directionArrowOffsetX, self.titleLabel.maxY + titleOffset);
    self.typeLabel.textAlignment = NSTextAlignmentLeft;
    self.distanceLabel.origin = CGPointMake(directionArrowOffsetX, self.typeLabel.maxY + typeOffset);
    self.distanceLabel.textAlignment = NSTextAlignmentLeft;
  }
  else
  {
    CGFloat const defaultWidth = size.width - 2. * minimumBorderOffset;
    [self resizeLabelsWithWidth:defaultWidth];
    CGFloat const titleOffset = IPAD ? 12. : 8.;
    CGFloat const arrowOffset = IPAD ? 80. : 40.;
    CGFloat const contentViewActualHeight = self.titleLabel.height + titleOffset + self.typeLabel.height + 2. * arrowOffset + kDirectionArrowSide + self.distanceLabel.height;
    CGFloat const contentViewSize = size.height > contentViewActualHeight ? contentViewActualHeight : size.height;
    CGFloat const yOffset = (size.height - contentViewSize) / 2.;
    self.contentView.frame = CGRectMake(minimumBorderOffset, yOffset, defaultWidth, contentViewSize);
    CGFloat const xOffset = self.contentView.width / 2.;
    self.titleLabel.origin = CGPointMake(xOffset - self.titleLabel.width / 2., 0.);
    self.titleLabel.textAlignment = NSTextAlignmentCenter;
    self.typeLabel.origin = CGPointMake(xOffset - self.typeLabel.width / 2., self.titleLabel.maxY + titleOffset);
    self.typeLabel.textAlignment = NSTextAlignmentCenter;
    self.directionArrow.center = CGPointMake(xOffset, self.typeLabel.maxY + arrowOffset + kDirectionArrowSide / 2.);
    self.distanceLabel.origin = CGPointMake(xOffset - self.distanceLabel.width / 2., self.directionArrow.maxY + arrowOffset);
    self.distanceLabel.textAlignment = NSTextAlignmentCenter;
  }
}

- (void)resizeLabelsWithWidth:(CGFloat)width
{
  self.titleLabel.width = self.typeLabel.width = self.distanceLabel.width = width;
  [self.titleLabel sizeToFit];
  [self.typeLabel sizeToFit];
  [self.distanceLabel sizeToFit];
}

- (void)setDirectionArrowTransform:(CGAffineTransform)transform
{
  self.directionArrow.transform = transform;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
  // Prevent super call to stop event propagation
  // [super touchesBegan:touches withEvent:event];
  [self.manager hideDirectionView];
}

@end
