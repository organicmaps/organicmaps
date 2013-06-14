#import "BalloonView.h"
#import <QuartzCore/CALayer.h>

#include "Framework.h"

#include "../../../gui/controller.hpp"

#include "../../../graphics/depth_constants.hpp"


@implementation BalloonView

@synthesize title;
@synthesize notes;
@synthesize color;
@synthesize editedBookmark;

@synthesize globalPosition;
@synthesize setName;


- (id) initWithTarget:(id)target
{
  if ((self = [super init]))
  {
    // default bookmark pin color
    self.color = @"placemark-red";

    Framework & f = GetFramework();

    // default bookmark name.
    self.title = [NSString stringWithUTF8String:f.GetBmCategory(f.LastEditedCategory())->GetName().c_str()];

    // load bookmarks from kml files
    f.LoadBookmarks();

    editedBookmark = MakeEmptyBookmarkAndCategory();
  }
  return self;
}

- (void) dealloc
{
  self.color = nil;
  self.title = nil;
  self.notes = nil;

  [super dealloc];
}

-(void)clear
{
  self.notes = nil;
  self.editedBookmark = MakeEmptyBookmarkAndCategory();
}

- (void) addOrEditBookmark
{
  if (IsValid(self.editedBookmark))
  {
    BookmarkCategory * bmCat = GetFramework().GetBmCategory(editedBookmark.first);
    if (bmCat)
    {
      Bookmark * bm = bmCat->GetBookmark(editedBookmark.second);
      bm->SetName([title UTF8String]);
      bm->SetType([color UTF8String]);
      if (notes)
        bm->SetDescription([notes UTF8String]);
      else
        bm->SetDescription([@"" UTF8String]);

      // Enable category visibility if it was turned off, so user can see newly added or edited bookmark
      if (!bmCat->IsVisible())
        bmCat->SetVisible(true);

      // Save all changes
      bmCat->SaveToKMLFile();
    }
  }
}

- (void) deleteBookmark
{
  if (IsValid(editedBookmark))
  {
    BookmarkCategory * cat = GetFramework().GetBmCategory(editedBookmark.first);
    if (cat)
    {
      cat->DeleteBookmark(editedBookmark.second);
      cat->SaveToKMLFile();
    }

    editedBookmark = MakeEmptyBookmarkAndCategory();
  }
}

- (void) addBookmarkToCategory:(size_t)index
{
  Framework & f = GetFramework();
  Bookmark bm(m2::PointD(globalPosition.x, globalPosition.y), [title UTF8String], [color UTF8String]);
  size_t const pos = f.AddBookmark(index, bm);

  self.editedBookmark = pair<int, int>(index, pos);
  self.setName = [NSString stringWithUTF8String:f.GetBmCategory(index)->GetName().c_str()];
}

@end
