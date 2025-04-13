#import "MWMTableViewCell.h"

@protocol MWMStreetEditorEditCellProtocol <NSObject>

- (void)editCellTextChanged:(NSString *)text;

@end

@interface MWMStreetEditorEditTableViewCell : MWMTableViewCell

- (void)configWithDelegate:(id<MWMStreetEditorEditCellProtocol>)delegate street:(NSString *)street;

@end
