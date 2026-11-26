#import "MWMEditorAddAdditionalNameTableViewCell.h"

@interface MWMEditorAddAdditionalNameTableViewCell ()

@property(weak, nonatomic) IBOutlet UILabel * label;

@end

@implementation MWMEditorAddAdditionalNameTableViewCell

- (void)config
{
  self.label.text = L(@"add_language");
  self.isAccessibilityElement = YES;
  self.accessibilityLabel = self.label.text;
  self.accessibilityTraits = UIAccessibilityTraitButton;
}

@end
