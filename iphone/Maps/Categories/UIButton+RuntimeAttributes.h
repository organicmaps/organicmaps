#import <UIKit/UIKit.h>

@interface UIButton (RuntimeAttributes)

@property(copy, nonatomic) NSString * localizedText;

- (void)setBackgroundColorName:(NSString *)colorName;
- (NSString *)backgroundColorName;
- (void)setBackgroundHighlightedColorName:(NSString *)colorName;
- (NSString *)backgroundHighlightedColorName;
- (void)setBackgroundSelectedColorName:(NSString *)colorName;
- (NSString *)backgroundSelectedColorName;
- (void)setTintColorName:(NSString *)colorName;
- (NSString *)tintColorName;

- (void)setBackgroundColor:(UIColor *)color forState:(UIControlState)state;

@end
