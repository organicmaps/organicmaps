@protocol MWMStreetEditorEditCellProtocol <NSObject>

- (void)editCellTextChanged:(NSString *)text;

@end

@interface MWMStreetEditorEditTableViewCell : UITableViewCell

- (void)configWithDelegate:(id<MWMStreetEditorEditCellProtocol>)delegate street:(NSString *)street;

@end
