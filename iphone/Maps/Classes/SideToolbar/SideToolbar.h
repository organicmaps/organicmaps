
#import <UIKit/UIKit.h>

@class SideToolbar;
@protocol SideToolbarDelegate <NSObject>

- (void)sideToolbar:(SideToolbar *)toolbar didPressButtonAtIndex:(NSInteger)buttonIndex;
- (void)sideToolbarDidPressBuyButton:(SideToolbar *)toolbar;
- (void)sideToolbarDidUpdateShift:(SideToolbar *)toolbar;

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
