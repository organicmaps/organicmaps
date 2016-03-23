#import "MWMEditorCommon.h"
#import "MWMTableViewCell.h"

@interface MWMEditorSwitchTableViewCell : MWMTableViewCell

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
                        on:(BOOL)on;

@end
