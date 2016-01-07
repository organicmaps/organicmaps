
@protocol MWMPlacePageOpeningHoursCellProtocol <NSObject>

- (BOOL)forcedButton;
- (BOOL)isPlaceholder;
- (BOOL)isEditor;
- (BOOL)openingHoursCellExpanded;
- (void)setOpeningHoursCellExpanded:(BOOL)openingHoursCellExpanded forCell:(UITableViewCell *)cell;

@end

@interface MWMPlacePageOpeningHoursCell : UITableViewCell

- (void)configWithDelegate:(id<MWMPlacePageOpeningHoursCellProtocol>)delegate
                      info:(NSString *)info
                  lastCell:(BOOL)lastCell;

- (CGFloat)cellHeight;

@end
