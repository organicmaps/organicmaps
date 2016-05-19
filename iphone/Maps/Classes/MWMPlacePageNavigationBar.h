@class MWMiPhonePortraitPlacePage;

@interface MWMPlacePageNavigationBar : SolidTouchView

+ (void)dismissNavigationBar;
+ (void)showNavigationBarForPlacePage:(MWMiPhonePortraitPlacePage *)placePage;
+ (void)remove;

- (instancetype)init __attribute__((unavailable("call navigationBarWithPlacePage: instead")));
- (instancetype)initWithCoder:(NSCoder *)aDecoder __attribute__((unavailable("call navigationBarWithPlacePage: instead")));
- (instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("call navigationBarWithPlacePage: instead")));

@end
