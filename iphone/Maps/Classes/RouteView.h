
#import <UIKit/UIKit.h>

typedef NS_ENUM(NSUInteger, RouteViewState) {
  RouteViewStateHidden,
  RouteViewStateInfo,
  RouteViewStateTurnInstructions,
};

@class RouteView;

@protocol RouteViewDelegate <NSObject>

- (void)routeViewDidCancelRouting:(RouteView *)routeView;
- (void)routeViewDidStartFollowing:(RouteView *)routeView;

- (void)routeViewWillEnterState:(RouteViewState)state;
- (void)routeViewDidEnterState:(RouteViewState)state;

@end

@interface RouteView : UIView

- (void)setState:(RouteViewState)state animated:(BOOL)animated;

- (void)updateWithInfo:(NSDictionary *)info;

@property (weak, nonatomic) id <RouteViewDelegate> delegate;
@property (nonatomic, readonly) RouteViewState state;

@end