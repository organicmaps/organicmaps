#import "MWMEditorCommon.h"

@interface MWMEditorSelectTableViewCell : UITableViewCell

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
               placeholder:(NSString *)placeholder;

@end
