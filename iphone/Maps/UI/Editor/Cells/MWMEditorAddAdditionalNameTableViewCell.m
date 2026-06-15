#import "MWMEditorAddAdditionalNameTableViewCell.h"

@interface MWMEditorAddAdditionalNameTableViewCell ()

@property(weak, nonatomic) IBOutlet UILabel * label;

@end

@implementation MWMEditorAddAdditionalNameTableViewCell

- (void)config
{
  self.label.text = L(@"add_language");
}

@end
