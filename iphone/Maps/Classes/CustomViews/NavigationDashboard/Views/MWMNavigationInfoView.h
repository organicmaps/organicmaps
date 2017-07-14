#import "MWMNavigationDashboardInfoProtocol.h"

enum class NavigationSearchState
{
  Maximized,
  MinimizedNormal,
  MinimizedSearch,
  MinimizedGas,
  MinimizedParking,
  MinimizedFood,
  MinimizedShop,
  MinimizedATM
};

typedef NS_ENUM(NSUInteger, MWMNavigationInfoViewState) {
  MWMNavigationInfoViewStateHidden,
  MWMNavigationInfoViewStatePrepare,
  MWMNavigationInfoViewStateNavigation
};

@interface MWMNavigationInfoView : UIView<MWMNavigationDashboardInfoProtocol>

@property(nonatomic) CGFloat topBound;
@property(nonatomic) CGFloat leftBound;
@property(nonatomic, readonly) NavigationSearchState searchState;
@property(nonatomic) MWMNavigationInfoViewState state;
@property(weak, nonatomic) UIView * ownerView;

- (void)setMapSearch;

- (void)updateToastView;

@end
