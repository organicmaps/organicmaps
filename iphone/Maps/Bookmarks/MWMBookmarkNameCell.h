#import "MWMTableViewCell.h"

@interface MWMBookmarkNameCell : MWMTableViewCell

- (void)configWithName:(NSString *)name delegate:(id<UITextFieldDelegate>)delegate;
- (NSString *)currentName;

@end
