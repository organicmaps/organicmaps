
#import <UIKit/UIKit.h>

@class SelectedColorView;
@protocol SelectedColorViewDelegate <NSObject>

- (void)selectedColorViewDidPress:(SelectedColorView *)selectedColorView;

@end

@interface SelectedColorView : UIView

- (void)setColor:(UIColor *)color;
@property (nonatomic, weak) id <SelectedColorViewDelegate> delegate;

@end
