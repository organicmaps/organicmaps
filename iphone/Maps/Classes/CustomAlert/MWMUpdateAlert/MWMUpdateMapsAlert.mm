#import "Common.h"
#import "MWMUpdateMapsAlert.h"
#import "Statistics.h"

static NSString * const kUpdateMapsAlertEventName = @"updateMapsAlertEvent";
static NSString * const kUpdateMapsAlertNibName = @"MWMUpdateMapsAlert";

@interface MWMUpdateMapsAlert ()

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * containerWidth;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * containerHeight;

@property (weak, nonatomic) IBOutlet UIImageView * image;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * imageMinHeight;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * imageHeight;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleTopOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleImageOffset;

@property (copy, nonatomic) TMWMVoidBlock okButtonAction;

@end

@implementation MWMUpdateMapsAlert

+ (instancetype)alertWithOkBlock:(TMWMVoidBlock)block
{
  [Statistics.instance
      logEvent:[NSString stringWithFormat:@"%@ - %@", kUpdateMapsAlertEventName, @"open"]];
  MWMUpdateMapsAlert * alert = [[[NSBundle mainBundle] loadNibNamed:kUpdateMapsAlertNibName
                                                              owner:self
                                                            options:nil] firstObject];
  alert.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  alert.okButtonAction = block;
  return alert;
}

- (void)setFrame:(CGRect)frame
{
  if (!isIOSVersionLessThan(8))
    [self updateForSize:frame.size];
  super.frame = frame;
}

- (void)updateForSize:(CGSize)size
{
  CGSize const iPadSize = {520.0, 600.0};
  CGSize const newSize = IPAD ? iPadSize : size;
  CGFloat const width = newSize.width;
  CGFloat const height = newSize.height;
  BOOL const hideImage = (self.imageHeight.multiplier * height <= self.imageMinHeight.constant);
  self.titleImageOffset.priority =
      hideImage ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;
  self.image.hidden = hideImage;
  self.containerWidth.constant = width;
  self.containerHeight.constant = height;
}

#pragma mark - Actions

- (IBAction)updateAllMapsAcrion
{
  if (self.okButtonAction)
    self.okButtonAction();
  [self close];
}

- (IBAction)notNowAction
{
  [self close];
}

#pragma mark - iOS 7 support methods

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
  UIView * superview = self.superview ? self.superview : UIApplication.sharedApplication.keyWindow;
  BOOL const isLandscape = UIInterfaceOrientationIsLandscape(orientation);
  CGFloat const minDim = MIN(superview.width, superview.height);
  CGFloat const maxDim = MAX(superview.width, superview.height);
  CGFloat const height = isLandscape ? minDim : maxDim;
  CGFloat const width = isLandscape ? maxDim : minDim;
  self.bounds = {{}, {width, height}};
}

- (void)setBounds:(CGRect)bounds
{
  if (isIOSVersionLessThan(8))
    [self updateForSize:bounds.size];
  super.bounds = bounds;
}

@end
