#import "MWMEditorTableViewHeader.h"

@interface MWMEditorTableViewHeader ()

@property (weak, nonatomic) IBOutlet UILabel * title;

@end

@implementation MWMEditorTableViewHeader

- (void)config:(NSString *)title
{
  self.title.text = title;
}

@end
