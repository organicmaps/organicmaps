@protocol MWMSideMenuButtonTapProtocol <NSObject>

- (void)handleSingleTap;
- (void)handleDoubleTap;

@end

@protocol MWMSideMenuButtonLayoutProtocol <NSObject>

- (void)menuButtonDidUpdateLayout;

@end