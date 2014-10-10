
#import <UIKit/UIKit.h>

@class RouteView;

@protocol RouteViewDelegate <NSObject>

- (void)routeViewDidCancelRouting:(RouteView *)routeView;
- (void)routeViewDidStartFollowing:(RouteView *)routeView;

@end

@interface RouteView : UIView

- (void)setVisible:(BOOL)visible animated:(BOOL)animated;
- (void)hideFollowButton;

- (void)updateDistance:(NSString *)distance withMetrics:(NSString *)metrics;

@property (nonatomic, weak) id <RouteViewDelegate> delegate;
@property (nonatomic, readonly) BOOL visible;

@end
