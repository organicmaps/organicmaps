#import "MWMStreetEditorEditTableViewCell.h"
#import "UITextField+RuntimeAttributes.h"

@interface MWMStreetEditorEditTableViewCell () <UITextFieldDelegate>

@property (weak, nonatomic) IBOutlet UITextField * textField;

@property (weak, nonatomic) id<MWMStreetEditorEditCellProtocol> delegate;

@end

@implementation MWMStreetEditorEditTableViewCell

- (void)configWithDelegate:(id<MWMStreetEditorEditCellProtocol>)delegate street:(NSString *)street
{
  self.delegate = delegate;
  self.textField.text = street;
}

#pragma mark - UITextFieldDelegate

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
  NSString * newString = [textField.text stringByReplacingCharactersInRange:range withString:string];
  [self.delegate editCellTextChanged:newString];
  return YES;
}

@end
