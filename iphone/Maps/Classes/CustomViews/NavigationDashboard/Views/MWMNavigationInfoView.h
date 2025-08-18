enum class NavigationSearchState
{
  Maximized,
  MinimizedNormal,
  MinimizedSearch,
  MinimizedGas,
  MinimizedParking,
  MinimizedEat,
  MinimizedFood,
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

- (void)setSearchState:(NavigationSearchState)searchState animated:(BOOL)animated;

- (void)onNavigationInfoUpdated:(MWMNavigationDashboardEntity *)info;

- (void)updateToastView;

@end
