@class MWMPlacePageViewManager;

@interface MWMPlacePageActionBar : SolidTouchView

@property (nonatomic) BOOL isBookmark;
@property (nonatomic) BOOL isPrepareRouteMode;

@property (weak, nonatomic) IBOutlet UIButton * shareButton;

+ (MWMPlacePageActionBar *)actionBarForPlacePageManager:(MWMPlacePageViewManager *)placePageManager;
- (void)configureWithPlacePageManager:(MWMPlacePageViewManager *)placePageManager;

- (instancetype)init __attribute__((unavailable("call actionBarForPlacePage: instead")));
- (instancetype)initWithCoder:(NSCoder *)aDecoder __attribute__((unavailable("call actionBarForPlacePage: instead")));
- (instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("call actionBarForPlacePage: instead")));

@end
