#import "MWMEditorSegmentedTableViewCell.hpp"
#import "SwiftBridge.h"

@interface MWMEditorSegmentedTableViewCell ()

@property(weak, nonatomic) IBOutlet UIImageView * icon;
@property(weak, nonatomic) IBOutlet UILabel * label;
@property(weak, nonatomic) IBOutlet UISegmentedControl * segmentedControl;

@property(weak, nonatomic) id<MWMEditorCellProtocol> delegate;

@end

@implementation MWMEditorSegmentedTableViewCell

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
                     value:(YesNoUnknown)value
{
  self.delegate = delegate;
  self.icon.image = icon;
  self.icon.styleName = @"MWMBlack";
  self.label.text = text;

  [self.segmentedControl setTitle:NSLocalizedString(@"no", nil) forSegmentAtIndex:0];
  [self.segmentedControl setTitle:NSLocalizedString(@"yes", nil) forSegmentAtIndex:2];

  switch (value)
  {
  case Yes: self.segmentedControl.selectedSegmentIndex = 2; break;
  case No: self.segmentedControl.selectedSegmentIndex = 0; break;
  case Unknown: self.segmentedControl.selectedSegmentIndex = 1; break;
  }

  [self setTextColorWithSegmentedValue:value];
}

- (void)setTextColorWithSegmentedValue:(YesNoUnknown)value
{
  switch (value)
  {
  case Yes:
  case No: self.label.textColor = [UIColor blackPrimaryText]; break;
  case Unknown: self.label.textColor = [UIColor blackHintText]; break;
  }
}

- (IBAction)valueChanged
{
  YesNoUnknown value;
  switch (self.segmentedControl.selectedSegmentIndex)
  {
  case 0: value = No; break;
  case 1: value = Unknown; break;
  case 2: value = Yes; break;
  default:
    value = Unknown;
    NSAssert(false, @"Unexpected YesNoUnknown value %ld",
             static_cast<long>(self.segmentedControl.selectedSegmentIndex));
  }

  [self.delegate cell:self changeSegmented:value];
  [self setTextColorWithSegmentedValue:value];
}

@end
