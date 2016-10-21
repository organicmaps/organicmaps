@class MWMPlacePageViewManager;
@class MWMPlacePageData;

@protocol MWMActionBarSharedData<NSObject>

- (BOOL)isBookmark;
- (BOOL)isOpentable;
- (BOOL)isBooking;
- (BOOL)isApi;
- (BOOL)isMyPosition;
- (NSString *)title;
- (NSString *)subtitle;
- (NSString *)phoneNumber;

@end

@protocol MWMActionBarProtocol;

@interface MWMPlacePageActionBar : SolidTouchView

+ (MWMPlacePageActionBar *)actionBarWithDelegate:(id<MWMActionBarProtocol>)delegate;
- (void)configureWithData:(id<MWMActionBarSharedData>)data;

@property (nonatomic) BOOL isBookmark;

- (UIView *)shareAnchor;
- (BOOL)isPrepareRouteMode;

+ (MWMPlacePageActionBar *)actionBarForPlacePageManager:(MWMPlacePageViewManager *)placePageManager;
- (void)configureWithPlacePageManager:(MWMPlacePageViewManager *)placePageManager;

- (instancetype)init __attribute__((unavailable("call actionBarForPlacePage: instead")));
- (instancetype)initWithCoder:(NSCoder *)aDecoder __attribute__((unavailable("call actionBarForPlacePage: instead")));
- (instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("call actionBarForPlacePage: instead")));

@end
