#import <UIKit/UIKit.h>

@protocol ColorPickerDelegate <NSObject>
-(void)colorPicked:(size_t)colorIndex;
@end

@interface ColorPickerView : UIView

@property (nonatomic, weak) id <ColorPickerDelegate> delegate;

- (id)initWithWidth:(CGFloat)width andSelectButton:(size_t)selectedIndex;
+ (UIColor *)buttonColor:(size_t)index;
+ (UIColor *)colorForName:(NSString *)name;
+ (NSString *)colorName:(size_t)index;
+ (size_t)getColorIndex:(NSString *)name;

@end
