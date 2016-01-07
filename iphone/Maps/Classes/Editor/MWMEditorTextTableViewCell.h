#import "MWMEditorCommon.h"

@interface MWMEditorTextTableViewCell : UITableViewCell

@property (nonatomic, readonly) NSString * text;

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
               placeholder:(NSString *)placeholder
                  lastCell:(BOOL)lastCell;

@end
