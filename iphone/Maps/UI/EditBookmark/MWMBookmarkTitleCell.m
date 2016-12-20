#import "MWMBookmarkTitleCell.h"

@interface MWMBookmarkTitleCell () <UITextFieldDelegate>

@property (weak, nonatomic) IBOutlet UITextField * name;
@property (weak, nonatomic) id<MWMBookmarkTitleDelegate> delegate;

@end

@implementation MWMBookmarkTitleCell

- (void)configureWithName:(NSString *)name delegate:(id<MWMBookmarkTitleDelegate>)delegate
{
  self.name.text = name;
  self.delegate = delegate;
}

- (void)textFieldDidEndEditing:(UITextField *)textField
{
  [self.delegate didFinishEditingBookmarkTitle:textField.text];
}

@end
