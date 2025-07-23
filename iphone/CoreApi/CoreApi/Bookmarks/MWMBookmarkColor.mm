#import "MWMBookmarkColor+Core.h"

MWMBookmarkColor convertKmlColor(kml::PredefinedColor kmlColor)
{
  switch (kmlColor)
  {
  case kml::PredefinedColor::None: return MWMBookmarkColorNone;
  case kml::PredefinedColor::Red: return MWMBookmarkColorRed;
  case kml::PredefinedColor::Blue: return MWMBookmarkColorBlue;
  case kml::PredefinedColor::Purple: return MWMBookmarkColorPurple;
  case kml::PredefinedColor::Yellow: return MWMBookmarkColorYellow;
  case kml::PredefinedColor::Pink: return MWMBookmarkColorPink;
  case kml::PredefinedColor::Brown: return MWMBookmarkColorBrown;
  case kml::PredefinedColor::Green: return MWMBookmarkColorGreen;
  case kml::PredefinedColor::Orange: return MWMBookmarkColorOrange;
  case kml::PredefinedColor::DeepPurple: return MWMBookmarkColorDeepPurple;
  case kml::PredefinedColor::LightBlue: return MWMBookmarkColorLightBlue;
  case kml::PredefinedColor::Cyan: return MWMBookmarkColorCyan;
  case kml::PredefinedColor::Teal: return MWMBookmarkColorTeal;
  case kml::PredefinedColor::Lime: return MWMBookmarkColorLime;
  case kml::PredefinedColor::DeepOrange: return MWMBookmarkColorDeepOrange;
  case kml::PredefinedColor::Gray: return MWMBookmarkColorGray;
  case kml::PredefinedColor::BlueGray: return MWMBookmarkColorBlueGray;
  case kml::PredefinedColor::Count: return MWMBookmarkColorCount;
  }
}
