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

@class MWMNavigationDashboardEntity;

@interface MWMNavigationInfoView : UIView

@property(nonatomic, readonly) NavigationSearchState searchState;
@property(nonatomic) MWMNavigationInfoViewState state;
@property(weak, nonatomic) UIView * ownerView;
@property(nonatomic) CGRect availableArea;

- (void)onNavigationInfoUpdated:(MWMNavigationDashboardEntity *)info;

- (void)setMapSearch;

- (void)updateToastView;

@end
