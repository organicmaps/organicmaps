#import "MWMEditorCommon.h"
#import "MWMTableViewCell.h"

@interface MWMNoteButtonCell : MWMTableViewCell

- (void)configureWithDelegate:(id<MWMEditorCellProtocol>)delegate title:(NSString *)title;

@end
