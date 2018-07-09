#import <UIKit/UIKit.h>

#include "kml/types.hpp"

@protocol ColorPickerDelegate <NSObject>
-(void)colorPicked:(size_t)colorIndex;
@end

@interface ColorPickerView : UIView

@property (nonatomic, weak) id <ColorPickerDelegate> delegate;

- (id)initWithWidth:(CGFloat)width andSelectButton:(size_t)selectedIndex;
+ (UIColor *)buttonColor:(size_t)index;
+ (UIColor *)getUIColor:(kml::PredefinedColor)color;
+ (kml::PredefinedColor)colorValue:(size_t)index;
+ (size_t)getColorIndex:(kml::PredefinedColor)color;

@end
