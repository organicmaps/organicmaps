#import <UIKit/UIKit.h>

#include "Framework.h"

@interface BalloonView : NSObject
{
  UITableViewCell * m_titleView;
  id m_target;
  SEL m_selector;

  Framework::AddressInfo m_addressInfo;
}

@property(nonatomic, retain) NSString * title;
// Currently contains automatically updated address info
@property(nonatomic, retain) NSString * description;
@property(nonatomic, retain) UIImageView * pinImage;
// Stores displayed bookmark icon file name
@property(nonatomic, retain) NSString * color;
// Stores last used bookmark Set name
@property(nonatomic, retain) NSString * setName;
@property(nonatomic, assign, readonly) BOOL isDisplayed;
@property(nonatomic, assign) CGPoint globalPosition;

- (id) initWithTarget:(id)target andSelector:(SEL)selector;
- (void) showInView:(UIView *)view atPoint:(CGPoint)pt;
- (void) updatePosition:(UIView *)view atPoint:(CGPoint)pt;
- (void) hide;

@end
