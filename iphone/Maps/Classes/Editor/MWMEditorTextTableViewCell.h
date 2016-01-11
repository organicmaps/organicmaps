#import "MWMEditorCommon.h"

@interface MWMEditorTextTableViewCell : UITableViewCell

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
               placeholder:(NSString *)placeholder
                  lastCell:(BOOL)lastCell;

@end
