#import "MWMEditorCommon.h"

@interface MWMEditorSwitchTableViewCell : UITableViewCell

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
                        on:(BOOL)on
                  lastCell:(BOOL)lastCell;

@end
