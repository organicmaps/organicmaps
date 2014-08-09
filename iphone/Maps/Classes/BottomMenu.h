
#import <UIKit/UIKit.h>

@class BottomMenu;
@protocol BottomMenuDelegate <NSObject>

- (void)bottomMenu:(BottomMenu *)menu didPressItemWithName:(NSString *)itemName appURL:(NSString *)appURL webURL:(NSString *)webURL;

@end

@interface BottomMenu : UIView

@property (weak) id <BottomMenuDelegate> delegate;

- (void)setMenuHidden:(BOOL)hidden animated:(BOOL)animated;
@property (readonly, nonatomic) BOOL menuHidden;

@end
