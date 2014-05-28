
#import <UIKit/UIKit.h>
#import "LocationButton.h"
#import "UIKitCategories.h"

@class ToolbarView;
@protocol ToolbarViewDelegate <NSObject>

- (void)toolbar:(ToolbarView *)toolbar didPressItemWithName:(NSString *)itemName;

@end

@interface ToolbarView : SolidTouchView

@property (nonatomic, weak) id <ToolbarViewDelegate> delegate;
@property (nonatomic) LocationButton * locationButton;

@end
