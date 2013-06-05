#import <UIKit/UIKit.h>

#include "../../../map/bookmark.hpp"
#include "../../../map/bookmark_balloon.hpp"


@interface BalloonView : NSObject
{
  shared_ptr<BookmarkBalloon> m_balloon;

  graphics::Image::Info m_images[2];
}

@property(nonatomic, retain) NSString * title;
// Currently displays bookmark description (notes)
@property(nonatomic, retain) NSString * notes;
// Stores displayed bookmark icon file name
@property(nonatomic, retain) NSString * color;
// If we clicked already existing bookmark, it will be here
@property(nonatomic, assign) BookmarkAndCategory editedBookmark;

@property(nonatomic, assign) CGPoint globalPosition;
@property(nonatomic, retain) NSString * setName;


- (id) initWithTarget:(id)target andSelector:(SEL)selector;
- (void) showInView;
- (void) hide;
- (void) clear;
- (BOOL) isDisplayed;

// Kosher method to add bookmark into the Framework.
// It automatically "edits" bookmark if it's already exists
- (void) addOrEditBookmark;
// Deletes bookmark if we were editing it (clicked on already added bm)
// and does nothing if called for "new", not added bookmark
- (void) deleteBookmark;

- (void) addBookmarkToCategory:(size_t)index;

@end
