#import "MWMPlacePageOpeningHoursDayView.h"
#import "SwiftBridge.h"

@interface MWMPlacePageOpeningHoursDayView ()

@property(weak, nonatomic) IBOutlet UILabel * label;
@property(weak, nonatomic) IBOutlet UILabel * openTime;
@property(weak, nonatomic) IBOutlet UILabel * compatibilityLabel;

@property(weak, nonatomic) IBOutlet UILabel * breakLabel;
@property(weak, nonatomic) IBOutlet UIStackView * breaksHolder;

@property(weak, nonatomic) IBOutlet UILabel * closedLabel;

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
  }
  else
  {
    self.breakLabel.hidden = NO;
    self.breaksHolder.hidden = NO;
    for (NSString * br in breaks)
    {
      UILabel * label = [[UILabel alloc] init];
      label.text = br;
      label.font = self.currentDay ? UIFont.regular12.dynamic : UIFont.light12.dynamic;
      label.adjustsFontForContentSizeCategory = YES;
      label.textColor = [UIColor blackSecondaryText];
      [label configureSingleLineAutoScaling];
      label.translatesAutoresizingMaskIntoConstraints = NO;
      [self.breaksHolder addArrangedSubview:label];
    }
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

@end
