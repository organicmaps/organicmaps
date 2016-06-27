@interface MWMSideButtonsView : UIView

@property (nonatomic) CGFloat topBound;
@property (nonatomic) CGFloat bottomBound;

@property (nonatomic) BOOL zoomHidden;

- (instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("initWithFrame is not available")));
- (instancetype)init __attribute__((unavailable("init is not available")));

- (void)setHidden:(BOOL)hidden animated:(BOOL)animated;

@end
