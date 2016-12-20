#import "MWMTableViewCell.h"

@protocol MWMPlacePageOpeningHoursCellProtocol <NSObject>

- (BOOL)forcedButton;
- (BOOL)isPlaceholder;
- (BOOL)isEditor;
- (BOOL)openingHoursCellExpanded;
- (void)setOpeningHoursCellExpanded:(BOOL)openingHoursCellExpanded;

@end

@interface MWMPlacePageOpeningHoursCell : MWMTableViewCell

@property (nonatomic, readonly) BOOL isClosed;

- (void)configWithDelegate:(id<MWMPlacePageOpeningHoursCellProtocol>)delegate
                      info:(NSString *)info;

- (CGFloat)cellHeight;

@end
