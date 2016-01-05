
@protocol MWMPlacePageOpeningHoursCellProtocol <NSObject>

- (BOOL)openingHoursCellExpanded;
- (void)setOpeningHoursCellExpanded:(BOOL)openingHoursCellExpanded forCell:(UITableViewCell *)cell;
- (void)editPlaceTime;

@end

@interface MWMPlacePageOpeningHoursCell : UITableViewCell

- (void)configWithInfo:(NSString *)info delegate:(id<MWMPlacePageOpeningHoursCellProtocol>)delegate;

- (CGFloat)cellHeight;

@end
