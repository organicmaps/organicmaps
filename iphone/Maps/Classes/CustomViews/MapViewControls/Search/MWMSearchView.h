
@class MWMSearchTabButtonsView;

@protocol MWMSearchViewProtocol <NSObject>

- (void)searchFrameUpdated:(CGRect)frame;

@end

@interface MWMSearchView : SolidTouchView

@property (nonatomic) BOOL isVisible;

@property (nonatomic) BOOL tabBarIsVisible;
@property (nonatomic) BOOL compact;

@property (weak, nonatomic) id <MWMSearchViewProtocol> delegate;

@end
