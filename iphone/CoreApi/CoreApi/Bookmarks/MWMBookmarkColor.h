#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, MWMBookmarkColor) {
  MWMBookmarkColorNone = 0,
  MWMBookmarkColorRed,
  MWMBookmarkColorBlue,
  MWMBookmarkColorPurple,
  MWMBookmarkColorYellow,
  MWMBookmarkColorPink,
  MWMBookmarkColorBrown,
  MWMBookmarkColorGreen,
  MWMBookmarkColorOrange,

  // Extended colors.
  MWMBookmarkColorDeepPurple,
  MWMBookmarkColorLightBlue,
  MWMBookmarkColorCyan,
  MWMBookmarkColorTeal,
  MWMBookmarkColorLime,
  MWMBookmarkColorDeepOrange,
  MWMBookmarkColorGray,
  MWMBookmarkColorBlueGray,

  MWMBookmarkColorCount
} NS_SWIFT_NAME(BookmarkColor);
