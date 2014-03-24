
#import <UIKit/UIKit.h>

@class SideToolbar;
@protocol SideToolbarDelegate <NSObject>

- (void)sideToolbar:(SideToolbar *)toolbar didPressItemWithName:(NSString *)itemName;
- (void)sideToolbarDidUpdateShift:(SideToolbar *)toolbar;
- (void)sideToolbarWillOpenMenu:(SideToolbar *)toolbar;
- (void)sideToolbarWillCloseMenu:(SideToolbar *)toolbar;
- (void)sideToolbarDidCloseMenu:(SideToolbar *)toolbar;

@end

@interface SideToolbar : UIView

@property (nonatomic, weak) UIView * slideView;

@property (weak) id <SideToolbarDelegate> delegate;

- (void)setMenuHidden:(BOOL)hidden animated:(BOOL)animated;
@property (readonly, nonatomic) BOOL isMenuHidden;

@property (nonatomic) CGFloat menuShift;
@property (nonatomic, readonly) CGFloat minimumMenuShift;
@property (nonatomic, readonly) CGFloat maximumMenuShift;

@end
