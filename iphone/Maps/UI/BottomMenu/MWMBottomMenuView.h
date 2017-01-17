typedef NS_ENUM(NSUInteger, MWMBottomMenuState)
{
  MWMBottomMenuStateHidden,
  MWMBottomMenuStateInactive,
  MWMBottomMenuStateActive,
  MWMBottomMenuStateCompact,
  MWMBottomMenuStatePlanning,
  MWMBottomMenuStateGo,
  MWMBottomMenuStateRoutingError,
  MWMBottomMenuStateRouting,
  MWMBottomMenuStateRoutingExpanded
};

@class MWMTaxiCollectionView;

@interface MWMBottomMenuView : SolidTouchView

@property(nonatomic) MWMBottomMenuState state;
@property(nonatomic) MWMBottomMenuState restoreState;

@property(nonatomic) CGFloat leftBound;
@property(nonatomic) CGFloat layoutThreshold;

@property(weak, nonatomic, readonly) IBOutlet MWMTaxiCollectionView * taxiCollectionView;

@property(nonatomic) BOOL searchIsActive;

- (void)refreshLayout;

@end
