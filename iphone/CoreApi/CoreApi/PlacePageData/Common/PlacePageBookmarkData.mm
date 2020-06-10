#import "PlacePageBookmarkData+Core.h"

static MWMBookmarkColor convertKmlColor(kml::PredefinedColor kmlColor) {
  switch (kmlColor) {
    case kml::PredefinedColor::None:
      return MWMBookmarkColorNone;
    case kml::PredefinedColor::Red:
      return MWMBookmarkColorRed;
    case kml::PredefinedColor::Blue:
      return MWMBookmarkColorBlue;
    case kml::PredefinedColor::Purple:
      return MWMBookmarkColorPurple;
    case kml::PredefinedColor::Yellow:
      return MWMBookmarkColorYellow;
    case kml::PredefinedColor::Pink:
      return MWMBookmarkColorPink;
    case kml::PredefinedColor::Brown:
      return MWMBookmarkColorBrown;
    case kml::PredefinedColor::Green:
      return MWMBookmarkColorGreen;
    case kml::PredefinedColor::Orange:
      return MWMBookmarkColorOrange;
    case kml::PredefinedColor::DeepPurple:
      return MWMBookmarkColorDeepPurple;
    case kml::PredefinedColor::LightBlue:
      return MWMBookmarkColorLightBlue;
    case kml::PredefinedColor::Cyan:
      return MWMBookmarkColorCyan;
    case kml::PredefinedColor::Teal:
      return MWMBookmarkColorTeal;
    case kml::PredefinedColor::Lime:
      return MWMBookmarkColorLime;
    case kml::PredefinedColor::DeepOrange:
      return MWMBookmarkColorDeepOrange;
    case kml::PredefinedColor::Gray:
      return MWMBookmarkColorGray;
    case kml::PredefinedColor::BlueGray:
      return MWMBookmarkColorBlueGray;
    case kml::PredefinedColor::Count:
      return MWMBookmarkColorCount;
  }
}

@implementation PlacePageBookmarkData

@end

@implementation PlacePageBookmarkData (Core)

- (instancetype)initWithRawData:(place_page::Info const &)rawData {
  self = [super init];
  if (self) {
    _bookmarkId = rawData.GetBookmarkId();
    _bookmarkGroupId = rawData.GetBookmarkCategoryId();
    _externalTitle = rawData.GetSecondaryTitle().empty() ? nil : @(rawData.GetSecondaryTitle().c_str());
    _bookmarkDescription = rawData.IsBookmark() ? @(GetPreferredBookmarkStr(rawData.GetBookmarkData().m_description).c_str()) : nil;
    _bookmarkCategory = rawData.IsBookmark() ? @(rawData.GetBookmarkCategoryName().c_str()) : nil;
    _isHtmlDescription = strings::IsHTML(GetPreferredBookmarkStr(rawData.GetBookmarkData().m_description));
    _isEditable = GetFramework().GetBookmarkManager().IsEditableBookmark(_bookmarkId);
    _color = convertKmlColor(rawData.GetBookmarkData().m_color.m_predefinedColor);
  }
  return self;
}

@end
