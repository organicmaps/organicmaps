@protocol MWMEditorCellProtocol;

@interface MWMEditorCategoryCell : UITableViewCell

- (void)configureWithDelegate:(id<MWMEditorCellProtocol>)delegate detailTitle:(NSString *)detail isCreating:(BOOL)isCreating;

@end
