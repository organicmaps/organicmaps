#import "MWMEditorCommon.h"

@interface MWMEditorTextTableViewCell : UITableViewCell

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
               placeholder:(NSString *)placeholder
              keyboardType:(UIKeyboardType)keyboardType
                  lastCell:(BOOL)lastCell;

@end
