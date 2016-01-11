@protocol MWMStreetEditorCommonTableViewCellProtocol <NSObject>

- (void)selectCell:(UITableViewCell *)selectedCell;

@end

@interface MWMStreetEditorCommonTableViewCell : UITableViewCell

- (void)configWithDelegate:(id<MWMStreetEditorCommonTableViewCellProtocol>)delegate street:(NSString *)street selected:(BOOL)selected;

@end
