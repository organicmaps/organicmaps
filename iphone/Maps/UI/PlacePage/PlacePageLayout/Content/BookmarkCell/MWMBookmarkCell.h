#import "MWMTableViewCell.h"

@protocol MWMPlacePageButtonsProtocol
, MWMPlacePageCellUpdateProtocol;

@interface MWMBookmarkCell : MWMTableViewCell

- (void)configureWithText:(NSString *)text
       updateCellDelegate:(id<MWMPlacePageCellUpdateProtocol>)updateCellDelegate
     editBookmarkDelegate:(id<MWMPlacePageButtonsProtocol>)editBookmarkDelegate
                   isHTML:(BOOL)isHTML
               isEditable:(BOOL)isEditable;

@end
