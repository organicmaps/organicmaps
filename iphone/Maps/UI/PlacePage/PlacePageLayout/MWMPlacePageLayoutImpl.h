#import "MWMPlacePageActionBar.h"
#import "MWMPPView.h"

namespace place_page_layout
{
NSTimeInterval const kAnimationSpeed = IPAD ? 0.15 : 0.25;

inline void animate(MWMVoidBlock animate, MWMVoidBlock completion = nil)
{
  [UIView animateWithDuration:kAnimationSpeed
                        delay:0
                      options:UIViewAnimationOptionCurveEaseIn
                   animations:^{
                     animate();
                   }
                   completion:^(BOOL finished) {
                     if (completion)
                       completion();
                   }];
}
}  // namepsace place_page_layout

@protocol MWMPlacePageLayoutDelegate;
@class MWMPPPreviewLayoutHelper;

@protocol MWMPlacePageLayoutImpl <NSObject>

- (instancetype)initOwnerView:(UIView *)ownerView
                placePageView:(MWMPPView *)placePageView
                     delegate:(id<MWMPlacePageLayoutDelegate>)delegate;
- (void)onShow;
- (void)onClose;
- (void)onScreenResize:(CGSize const &)size;
- (void)onUpdatePlacePageWithHeight:(CGFloat)height;

@property(weak, nonatomic) UIView * ownerView;
@property(weak, nonatomic) MWMPPView * placePageView;
@property(weak, nonatomic) id<MWMPlacePageLayoutDelegate> delegate;
@property(weak, nonatomic) MWMPlacePageActionBar * actionBar;

@optional
- (void)updateLayoutWithTopBound:(CGFloat)topBound;
- (void)updateLayoutWithLeftBound:(CGFloat)leftBound;
- (void)setInitialTopBound:(CGFloat)topBound leftBound:(CGFloat)leftBound;
- (void)setPreviewLayoutHelper:(MWMPPPreviewLayoutHelper *)layoutHelper;

@end
