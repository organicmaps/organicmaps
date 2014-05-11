
#import <UIKit/UIKit.h>

@class BottomMenu;
@protocol BottomMenuDelegate <NSObject>

- (void)bottomMenu:(BottomMenu *)menu didPressItemWithName:(NSString *)itemName;
- (void)bottomMenuDidPressBuyButton:(BottomMenu *)menu;

@end

@interface BottomMenu : UIView

@property (weak) id <BottomMenuDelegate> delegate;

- (instancetype)initWithFrame:(CGRect)frame items:(NSArray *)items;

- (void)setMenuHidden:(BOOL)hidden animated:(BOOL)animated;
@property (readonly, nonatomic) BOOL menuHidden;

@end
