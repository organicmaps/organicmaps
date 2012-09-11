#import <UIKit/UIKit.h>

#include "Framework.h"

@interface BalloonView : NSObject
{
  UIImageView * m_titleView;
  id m_target;
  SEL m_selector;

  Framework::AddressInfo m_addressInfo;
  // If we clicked already existing bookmark, it will be here
  BookmarkAndCategory m_editedBookmark;
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
- (void) showInView:(UIView *)view atPoint:(CGPoint)pt withBookmark:(BookmarkAndCategory)bm;
- (void) updatePosition:(UIView *)view atPoint:(CGPoint)pt;
- (void) hide;
// Update baloon image with new title
- (void) updateTitle:(NSString *)newTitle;

// Kosher method to add bookmark into the Framework.
// It automatically "edits" bookmark if it's already exists
- (void) addOrEditBookmark;
// Deletes bookmark if we were editing it (clicked on already added bm)
// and does nothing if called for "new", not added bookmark
- (void) deleteBookmark;

@end
