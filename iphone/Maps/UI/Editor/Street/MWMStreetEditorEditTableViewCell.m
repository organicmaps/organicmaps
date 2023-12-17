#import "MWMStreetEditorEditTableViewCell.h"

@interface MWMStreetEditorEditTableViewCell ()

@property (weak, nonatomic) IBOutlet UITextField * textField;
@property (weak, nonatomic) id<MWMStreetEditorEditCellProtocol> delegate;

@end

@implementation MWMStreetEditorEditTableViewCell

- (void)configWithDelegate:(id<MWMStreetEditorEditCellProtocol>)delegate street:(NSString *)street
{
  self.delegate = delegate;
  self.textField.text = street;
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(textDidChange:)
                                               name:UITextFieldTextDidChangeNotification
                                             object:self.textField];
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)textDidChange:(NSNotification*)notification
{
  UITextField * textField = (UITextField *)[notification object];
  [self.delegate editCellTextChanged:textField.text];
}

@end
