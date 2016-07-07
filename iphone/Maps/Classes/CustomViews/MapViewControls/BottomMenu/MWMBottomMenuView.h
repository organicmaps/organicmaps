
typedef NS_ENUM(NSUInteger, MWMBottomMenuState)
{
  MWMBottomMenuStateHidden,
  MWMBottomMenuStateInactive,
  MWMBottomMenuStateActive,
  MWMBottomMenuStateCompact,
  MWMBottomMenuStatePlanning,
  MWMBottomMenuStateGo,
  MWMBottomMenuStateRouting,
  MWMBottomMenuStateRoutingExpanded
};

@interface MWMBottomMenuView : SolidTouchView

@property(nonatomic) MWMBottomMenuState state;
@property(nonatomic) MWMBottomMenuState restoreState;

@property(nonatomic) CGFloat leftBound;
@property(nonatomic) CGFloat layoutThreshold;

@property(nonatomic) BOOL searchIsActive;

- (void)refreshLayout;

@end
