
#import <UIKit/UIKit.h>
#import "LocationButton.h"

@class ToolbarView;
@protocol ToolbarViewDelegate <NSObject>

- (void)toolbar:(ToolbarView *)toolbar didPressItemWithName:(NSString *)itemName;

@end

@interface ToolbarView : UIView

@property (nonatomic, weak) id <ToolbarViewDelegate> delegate;
@property (nonatomic) LocationButton * locationButton;

@end
