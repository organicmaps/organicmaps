#import <UIKit/UIKit.h>

IB_DESIGNABLE
@interface MWMRouteTypeButton : UIButton

@property (nonatomic, getter=isSelected) BOOL selected;
@property (nonatomic) IBInspectable UIImage * icon;
@property (nonatomic) IBInspectable UIImage * highlightedIcon;
@property (nonatomic) IBInspectable UIImage * selectedIcon;

- (void)startAnimating;
- (void)stopAnimating;

@end
