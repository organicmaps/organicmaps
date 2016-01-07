@protocol MWMCuisineEditorTableViewCellProtocol <NSObject>

- (void)change:(NSString *)key selected:(BOOL)selected;

@end

@interface MWMCuisineEditorTableViewCell : UITableViewCell

- (void)configWithDelegate:(id<MWMCuisineEditorTableViewCellProtocol>)delegate key:(NSString *)key selected:(BOOL)selected;

@end
