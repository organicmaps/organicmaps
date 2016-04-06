#import "MWMTableViewCell.h"

@protocol MWMEditorCellProtocol;

@interface MWMEditorTextTableViewCell : MWMTableViewCell

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
               placeholder:(NSString *)placeholder
              keyboardType:(UIKeyboardType)keyboardType;

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
               placeholder:(NSString *)placeholder
              errorMessage:(NSString *)errorMessage
                   isValid:(BOOL)isValid
              keyboardType:(UIKeyboardType)keyboardType;

@property (weak, nonatomic, readonly) IBOutlet UITextField * textField;

@end
