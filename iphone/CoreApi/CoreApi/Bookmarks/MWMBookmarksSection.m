#import "MWMBookmarksSection.h"

@implementation MWMBookmarksSection

- (instancetype)initWithTitle:(NSString *)title
                    bookmarks:(NSArray<MWMBookmark *> *)bookmarks
                       tracks:(NSArray<MWMTrack *> *)tracks {
  self = [super init];
  if (self) {
    _sectionName = title;
    _bookmarks = bookmarks;
    _tracks = tracks;
  }
  return self;
}

@end


