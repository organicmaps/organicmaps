#import "MWMBookmarkColor+Core.h"
#import "PlacePageBookmarkData+Core.h"

@implementation PlacePageBookmarkData

@end

@implementation PlacePageBookmarkData (Core)

- (instancetype)initWithRawData:(place_page::Info const &)rawData
{
  self = [super init];
  if (self)
  {
    _bookmarkId = rawData.GetBookmarkId();
    _bookmarkGroupId = rawData.GetBookmarkCategoryId();
    _externalTitle = rawData.GetSecondaryTitle().empty() ? nil : @(rawData.GetSecondaryTitle().c_str());
    _bookmarkDescription =
        rawData.IsBookmark() ? @(GetPreferredBookmarkStr(rawData.GetBookmarkData().m_description).c_str()) : nil;
    _bookmarkCategory = rawData.IsBookmark() ? @(rawData.GetBookmarkCategoryName().c_str()) : nil;
    _isHtmlDescription = strings::IsHTML(GetPreferredBookmarkStr(rawData.GetBookmarkData().m_description));
    _color = convertKmlColor(rawData.GetBookmarkData().m_color.m_predefinedColor);
  }
  return self;
}

@end
