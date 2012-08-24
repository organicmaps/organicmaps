#import <UIKit/UIKit.h>

#include "Framework.h"

@interface BalloonView : NSObject
{
  UIImageView * m_titleView;
  id m_target;
  SEL m_selector;

  Framework::AddressInfo m_addressInfo;
}

@property(nonatomic, retain) NSString * title;
// Currently contains automatically updated address info
@property(nonatomic, retain) NSString * description;
// Contains feature type(s)
@property(nonatomic, retain) NSString * type;
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
// Update baloon image with new title
- (void) updateTitle:(NSString *)newTitle;

@end
