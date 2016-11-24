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

@interface MWMNavigationInfoView : UIView<MWMNavigationDashboardInfoProtocol>

@property(nonatomic) CGFloat leftBound;
@property(nonatomic, readonly) CGFloat leftHeight;
@property(nonatomic, readonly) CGFloat rightHeight;
@property(nonatomic, readonly) NavigationSearchState searchState;

- (void)addToView:(UIView *)superview;
- (void)remove;

- (void)setMapSearch;

@end
