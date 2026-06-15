@interface MWMPlacePageOpeningHoursDayView : UIView

@property(nonatomic) BOOL currentDay;
@property(nonatomic) BOOL isCompatibility;

- (void)setLabelText:(NSString *)text isRed:(BOOL)isRed;
- (void)setOpenTimeText:(NSString *)text;
- (void)setBreaks:(NSArray<NSString *> *)breaks;
- (void)setClosed:(BOOL)closed;
- (void)setCompatibilityText:(NSString *)text isPlaceholder:(BOOL)isPlaceholder;

@end
