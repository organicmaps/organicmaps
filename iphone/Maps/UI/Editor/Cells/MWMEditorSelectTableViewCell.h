#import "MWMEditorCommon.h"
#import "MWMTableViewCell.h"

@interface MWMEditorSelectTableViewCell : MWMTableViewCell

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
               placeholder:(NSString *)placeholder;

@end
