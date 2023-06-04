#import "MWMTableViewCell.h"

@protocol MWMEditorCellProtocol;

@interface MWMEditorCategoryCell : MWMTableViewCell

- (void)configureWithDelegate:(id<MWMEditorCellProtocol>)delegate
                  detailTitle:(NSString *)detail
                   isCreating:(BOOL)isCreating;

@end
