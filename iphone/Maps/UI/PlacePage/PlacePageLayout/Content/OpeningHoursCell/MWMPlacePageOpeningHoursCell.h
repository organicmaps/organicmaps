#import "MWMTableViewCell.h"

@protocol MWMPlacePageOpeningHoursCellProtocol <NSObject>

- (BOOL)forcedButton;
- (BOOL)isPlaceholder;
- (BOOL)openingHoursCellExpanded;
- (void)setOpeningHoursCellExpanded:(BOOL)openingHoursCellExpanded;

@end

@interface MWMPlacePageOpeningHoursCell : MWMTableViewCell

- (void)configWithDelegate:(id<MWMPlacePageOpeningHoursCellProtocol>)delegate info:(NSString *)info;

@end
