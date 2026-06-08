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
    std::string const description = GetPreferredBookmarkStr(rawData.GetBookmarkData().m_description);
    _bookmarkDescription = rawData.IsBookmark() ? @(description.c_str()) : nil;
    _bookmarkCategory = rawData.IsBookmark() ? @(rawData.GetBookmarkCategoryName().c_str()) : nil;
    _isHtmlDescription = strings::IsHTML(description);
    auto const color = kml::GetEffectiveColor(rawData.GetBookmarkData().m_color);
    _color = [UIColor colorWithRed:color.GetRedF() green:color.GetGreenF() blue:color.GetBlueF() alpha:1.f];
  }
  return self;
}

@end
