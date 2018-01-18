#import "MWMCircularProgressState.h"

@class MWMPlacePageData;

@protocol MWMActionBarSharedData<NSObject>

- (BOOL)isBookmark;
- (BOOL)isOpentable;
- (BOOL)isPartner;
- (BOOL)isBooking;
- (BOOL)isBookingSearch;
- (BOOL)isApi;
- (BOOL)isMyPosition;
- (BOOL)isRoutePoint;
- (NSString *)title;
- (NSString *)subtitle;
- (NSString *)phoneNumber;
- (int)partnerIndex;
- (NSURL *)sponsoredURL;

@end

@protocol MWMActionBarProtocol;

@interface MWMPlacePageActionBar : SolidTouchView

+ (MWMPlacePageActionBar *)actionBarWithDelegate:(id<MWMActionBarProtocol>)delegate;
- (void)configureWithData:(id<MWMActionBarSharedData>)data;

@property(nonatomic) BOOL isBookmark;
@property(nonatomic) BOOL isAreaNotDownloaded;

- (void)setVisible:(BOOL)visible;
- (void)setDownloadingProgress:(CGFloat)progress;
- (void)setDownloadingState:(MWMCircularProgressState)state;

- (UIView *)shareAnchor;

- (instancetype)init __attribute__((unavailable("call actionBarForPlacePage: instead")));
- (instancetype)initWithCoder:(NSCoder *)aDecoder __attribute__((unavailable("call actionBarForPlacePage: instead")));
- (instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("call actionBarForPlacePage: instead")));

@end
