#import "MWMTableViewCell.h"

@protocol MWMPlacePageCellUpdateProtocol;

@interface MWMOpeningHoursCell : MWMTableViewCell

- (void)configureWithOpeningHours:(NSString *)openingHours
             updateLayoutDelegate:(id<MWMPlacePageCellUpdateProtocol>)delegate
                      isClosedNow:(BOOL)isClosedNow;
@end
