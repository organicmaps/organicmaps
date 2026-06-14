#import "MWMPlacePageOpeningHoursDayView.h"
#import "SwiftBridge.h"

@interface MWMPlacePageOpeningHoursDayView ()

@property(weak, nonatomic) IBOutlet UILabel * label;
@property(weak, nonatomic) IBOutlet UILabel * openTime;
@property(weak, nonatomic) IBOutlet UILabel * compatibilityLabel;

@property(weak, nonatomic) IBOutlet UILabel * breakLabel;
@property(weak, nonatomic) IBOutlet UIStackView * breaksHolder;

@property(weak, nonatomic) IBOutlet UILabel * closedLabel;

@property(weak, nonatomic) IBOutlet NSLayoutConstraint * height;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * labelTopSpacing;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * labelWidth;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * breakLabelWidth;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * breaksHolderHeight;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * openTimeLabelLeadingOffset;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * labelOpenTimeLabelSpacing;

@end

@implementation MWMPlacePageOpeningHoursDayView

- (void)setLabelText:(NSString *)text isRed:(BOOL)isRed
{
  UILabel * label = self.label;
  label.text = text;
  if (isRed)
    [label setStyleNameAndApply:@"regular16:redText"];
  else if (self.currentDay)
    [label setStyleNameAndApply:@"regular16:blackPrimaryText"];
  else
    [label setStyleNameAndApply:@"regular16:blackSecondaryText"];
}

- (void)setOpenTimeText:(NSString *)text
{
  self.openTime.hidden = (text.length == 0);
  self.openTime.text = text;
}

- (void)setBreaks:(NSArray<NSString *> *)breaks
{
  for (UIView * view in self.breaksHolder.arrangedSubviews)
  {
    [self.breaksHolder removeArrangedSubview:view];
    [view removeFromSuperview];
  }

  if (breaks.count == 0)
  {
    self.breakLabel.hidden = YES;
    self.breaksHolder.hidden = YES;
    self.breaksHolderHeight.constant = 0.0;
  }
  else
  {
    CGFloat breakSpacerHeight = 4.0;
    self.breakLabel.hidden = NO;
    self.breaksHolder.hidden = NO;
    CGFloat labelY = 0.0;
    for (NSString * br in breaks)
    {
      UILabel * label = [[UILabel alloc] initWithFrame:CGRectMake(0, labelY, 0, 0)];
      label.text = br;
      label.font = self.currentDay ? UIFont.regular12.dynamic : UIFont.light12.dynamic;
      label.adjustsFontForContentSizeCategory = YES;
      label.textColor = [UIColor blackSecondaryText];
      [label configureSingleLineAutoScaling];
      label.translatesAutoresizingMaskIntoConstraints = NO;
      [self.breaksHolder addArrangedSubview:label];
      CGSize const fittingSize = [label sizeThatFits:CGSizeMake(self.breaksHolder.width, CGFLOAT_MAX)];
      labelY += fittingSize.height + breakSpacerHeight;
    }
    self.breaksHolderHeight.constant = MAX(0.0, ceil(labelY - breakSpacerHeight));
  }
}

- (void)setClosed:(BOOL)closed
{
  self.closedLabel.hidden = !closed;
}

- (void)setCompatibilityText:(NSString *)text isPlaceholder:(BOOL)isPlaceholder
{
  self.compatibilityLabel.text = text;
  self.compatibilityLabel.textColor = isPlaceholder ? [UIColor blackHintText] : [UIColor blackPrimaryText];
}

- (void)invalidate
{
  [self setNeedsLayout];
  [self layoutIfNeeded];

  CGFloat const fittingWidth = self.width > 0.0 ? self.width : UIScreen.mainScreen.bounds.size.width;
  CGSize const targetSize = CGSizeMake(fittingWidth, UILayoutFittingCompressedSize.height);
  CGSize const fittingSize = [self systemLayoutSizeFittingSize:targetSize
                                 withHorizontalFittingPriority:UILayoutPriorityRequired
                                       verticalFittingPriority:UILayoutPriorityFittingSizeLevel];
  self.viewHeight = ceil(fittingSize.height);
}

#pragma mark - Properties

- (void)setViewHeight:(CGFloat)viewHeight
{
  _viewHeight = viewHeight;
  if (self.currentDay)
  {
    self.height.constant = viewHeight;
  }
  else
  {
    CGRect frame = self.frame;
    frame.size.height = viewHeight;
    self.frame = frame;
  }
}

- (void)setIsCompatibility:(BOOL)isCompatibility
{
  _isCompatibility = isCompatibility;
  self.compatibilityLabel.hidden = !isCompatibility;
  self.label.hidden = isCompatibility;
  self.openTime.hidden = isCompatibility;
  self.breakLabel.hidden = isCompatibility;
  self.breaksHolder.hidden = isCompatibility;
  self.closedLabel.hidden = isCompatibility;
}

- (CGFloat)openTimeLeadingOffset
{
  return self.openTime.minX;
}

- (void)setOpenTimeLeadingOffset:(CGFloat)openTimeLeadingOffset
{
  self.openTimeLabelLeadingOffset.constant = openTimeLeadingOffset;
}

@end
