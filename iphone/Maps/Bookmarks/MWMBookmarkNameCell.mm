#import "MWMBookmarkNameCell.h"

@interface MWMBookmarkNameCell ()

@property(weak, nonatomic) IBOutlet UITextField * nameField;

@end

@implementation MWMBookmarkNameCell

- (void)configWithName:(NSString *)name delegate:(id<UITextFieldDelegate>)delegate
{
  self.nameField.text = name;
  self.nameField.delegate = delegate;
}

- (NSString *)currentName { return self.nameField.text; }

@end
