@class MWMPlacePageViewManager;

@interface MWMPlacePageActionBar : SolidTouchView

@property (nonatomic) BOOL isBookmark;

- (UIView *)shareAnchor;
- (BOOL)isPrepareRouteMode;

+ (MWMPlacePageActionBar *)actionBarForPlacePageManager:(MWMPlacePageViewManager *)placePageManager;
- (void)configureWithPlacePageManager:(MWMPlacePageViewManager *)placePageManager;

- (instancetype)init __attribute__((unavailable("call actionBarForPlacePage: instead")));
- (instancetype)initWithCoder:(NSCoder *)aDecoder __attribute__((unavailable("call actionBarForPlacePage: instead")));
- (instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("call actionBarForPlacePage: instead")));

@end
