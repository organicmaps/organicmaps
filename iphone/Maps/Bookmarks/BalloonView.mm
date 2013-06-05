#import "BalloonView.h"
#import <QuartzCore/CALayer.h>

#include "Framework.h"

#include "../../../gui/controller.hpp"


@implementation BalloonView

@synthesize title;
@synthesize notes;
@synthesize color;
@synthesize editedBookmark;

@synthesize globalPosition;
@synthesize setName;


- (id) initWithTarget:(id)target andSelector:(SEL)selector;
{
  if ((self = [super init]))
  {
    // default bookmark pin color
    self.color = @"placemark-red";

    Framework & f = GetFramework();

    // default bookmark name.
    self.title = [NSString stringWithUTF8String:f.GetBmCategory(f.LastEditedCategory())->GetName().c_str()];

    // Init balloon.
    BookmarkBalloon::Params bp;
    bp.m_position = graphics::EPosAbove;
    bp.m_depth = graphics::maxDepth;
    bp.m_pivot = m2::PointD(0, 0);
    bp.m_mainText = "Bookmark";
    bp.m_framework = &f;

    m_balloon.reset(new BookmarkBalloon(bp));
    m_balloon->setIsVisible(false);

    typedef void (*fireBalloonFnT)(id, SEL);
    fireBalloonFnT fn = (fireBalloonFnT)[target methodForSelector:selector];
    m_balloon->setOnClickListener(bind(fn, target, selector));

    f.GetGuiController()->AddElement(m_balloon);

    graphics::EDensity const density = graphics::EDensityMDPI;//f.GetRenderPolicy()->Density();
    m_images[0] = graphics::Image::Info("plus.png", density);
    m_images[1] = graphics::Image::Info("arrow.png", density);

    [self updateBalloonSize];

    // load bookmarks from kml files
    f.LoadBookmarks();

    editedBookmark = MakeEmptyBookmarkAndCategory();
  }
  return self;
}

- (void) updateBalloonSize
{
  ScreenBase const & s = GetFramework().GetNavigator().Screen();
  m_balloon->onScreenSize(s.GetWidth(), s.GetHeight());
}

- (void) dealloc
{
  self.color = nil;
  self.title = nil;
  self.notes = nil;

  [super dealloc];
}

- (void) showInView
{
  m_balloon->setImage(m_images[0]);
  m_balloon->setBookmarkCaption([title UTF8String], "");
  m_balloon->setGlbPivot(m2::PointD(globalPosition.x, globalPosition.y));

  [self updateBalloonSize];
  m_balloon->showAnimated();
}

- (void) hide
{
  m_balloon->hide();
}

-(void)clear
{
  self.notes = nil;
  self.editedBookmark = MakeEmptyBookmarkAndCategory();
}

- (BOOL) isDisplayed
{
  return m_balloon->isVisible();
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
