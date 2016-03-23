#import "MWMTableViewCell.h"

@protocol MWMEditorCellProtocol;

@interface MWMEditorTextTableViewCell : MWMTableViewCell

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
               placeholder:(NSString *)placeholder
              keyboardType:(UIKeyboardType)keyboardType;

@end
