#import "MWMTableViewCell.h"

@protocol MWMPlacePageCellUpdateProtocol;

@interface MWMOpeningHoursCell : MWMTableViewCell

- (void)configureWithOpeningHours:(NSString *)openningHours
             updateLayoutDelegate:(id<MWMPlacePageCellUpdateProtocol>)delegate
                      isClosedNow:(BOOL)isClosedNow;
@end
