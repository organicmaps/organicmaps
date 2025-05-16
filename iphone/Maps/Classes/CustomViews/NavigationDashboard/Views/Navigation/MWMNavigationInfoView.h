#import "RoutePreviewView.h"

typedef NS_ENUM(NSUInteger, NavigationSearchState) {
  NavigationSearchStateMaximized,
  NavigationSearchStateMinimizedNormal,
  NavigationSearchStateMinimizedSearch,
  NavigationSearchStateMinimizedGas,
  NavigationSearchStateMinimizedParking,
  NavigationSearchStateMinimizedEat,
  NavigationSearchStateMinimizedFood,
  NavigationSearchStateMinimizedATM
};

typedef NS_ENUM(NSUInteger, MWMNavigationInfoViewState) {
  MWMNavigationInfoViewStateHidden,
  MWMNavigationInfoViewStatePrepare,
  MWMNavigationInfoViewStateNavigation
};

@class MWMNavigationDashboardEntity;

NS_SWIFT_NAME(NavigationInfoView)
@interface MWMNavigationInfoView : UIView

@property(nonatomic, readonly) NavigationSearchState searchState;
@property(nonatomic) MWMNavigationInfoViewState state;
@property(weak, nonatomic) UIView * ownerView;
@property(nonatomic) CGRect availableArea;
@property(weak, nonatomic) id<RouteNavigationControlsDelegate> delegate;

- (void)setMapSearch;

- (void)setSearchState:(NavigationSearchState)searchState animated:(BOOL)animated;

- (void)onNavigationInfoUpdated:(MWMNavigationDashboardEntity *)info;

- (void)updateToastView;

- (void)updateSideButtonsAvailableArea:(CGRect)frame;

@end
