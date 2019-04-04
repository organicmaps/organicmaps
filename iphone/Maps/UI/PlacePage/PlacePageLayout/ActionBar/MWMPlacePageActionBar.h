#import "MWMCircularProgressState.h"

@class MWMPlacePageData;

@protocol MWMActionBarProtocol;

@interface MWMPlacePageActionBar : SolidTouchView

+ (MWMPlacePageActionBar *)actionBarWithDelegate:(id<MWMActionBarProtocol>)delegate;
- (void)configureWithData:(MWMPlacePageData *)data;

@property(nonatomic) BOOL isAreaNotDownloaded;

- (void)setVisible:(BOOL)visible;
- (void)setDownloadingProgress:(CGFloat)progress;
- (void)setDownloadingState:(MWMCircularProgressState)state;

- (UIView *)shareAnchor;

- (instancetype)init __attribute__((unavailable("call actionBarForPlacePage: instead")));
- (instancetype)initWithCoder:(NSCoder *)aDecoder __attribute__((unavailable("call actionBarForPlacePage: instead")));
- (instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("call actionBarForPlacePage: instead")));

@end
