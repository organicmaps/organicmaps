#import "MWMTableViewCell.h"

@protocol MWMStreetEditorCommonTableViewCellProtocol <NSObject>

- (void)selectCell:(UITableViewCell *)selectedCell;

@end

@interface MWMStreetEditorCommonTableViewCell : MWMTableViewCell

- (void)configWithDelegate:(id<MWMStreetEditorCommonTableViewCellProtocol>)delegate street:(NSString *)street selected:(BOOL)selected;

@end
