typedef NS_ENUM(NSUInteger, MWMPlacePageOpeningHoursDayViewMode)
{
  MWMPlacePageOpeningHoursDayViewModeRegular,
  MWMPlacePageOpeningHoursDayViewModeCompatibility,
  MWMPlacePageOpeningHoursDayViewModeEmpty
};

@interface MWMPlacePageOpeningHoursDayView : UIView

@property (nonatomic) BOOL currentDay;
@property (nonatomic) CGFloat viewHeight;
@property (nonatomic) MWMPlacePageOpeningHoursDayViewMode mode;

- (void)setLabelText:(NSString *)text isRed:(BOOL)isRed;
- (void)setOpenTimeText:(NSString *)text;
- (void)setBreaks:(NSArray<NSString *> *)breaks;
- (void)setClosed:(BOOL)closed;
- (void)setCanExpand:(BOOL)canExpand;
- (void)setCompatibilityText:(NSString *)text;

- (void)invalidate;

@end
