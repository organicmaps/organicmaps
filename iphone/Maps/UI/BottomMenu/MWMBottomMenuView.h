typedef NS_ENUM(NSUInteger, MWMBottomMenuState) {
  MWMBottomMenuStateHidden,
  MWMBottomMenuStateInactive,
  MWMBottomMenuStateActive
};

@interface MWMBottomMenuView : SolidTouchView

@property(nonatomic) MWMBottomMenuState state;

@property(nonatomic) CGFloat layoutThreshold;

- (void)refreshLayout;

- (void)updateAvailableArea:(CGRect)frame;
- (BOOL)isCompact;

@end
