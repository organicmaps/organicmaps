#import "partners_api/utm.hpp"

#import <Foundation/Foundation.h>

#import "MWMUTM.h"

static inline UTM toUTM(MWMUTM utm)
{
  switch (utm)
  {
  case MWMUTMNone: return UTM::None;
  case MWMUTMBookmarksPageCatalogButton: return UTM::BookmarksPageCatalogButton;
  case MWMUTMToolbarButton: return UTM::ToolbarButton;
  case MWMUTMDownloadMwmBanner: return UTM::DownloadMwmBanner;
  case MWMUTMLargeToponymsPlacepageGallery: return UTM::LargeToponymsPlacepageGallery;
  case MWMUTMSightseeingsPlacepageGallery: return UTM::SightseeingsPlacepageGallery;
  case MWMUTMDiscoveryPageGallery: return UTM::DiscoveryPageGallery;
  case MWMUTMTipsAndTricks: return UTM::TipsAndTricks;
  case MWMUTMBookingPromo: return UTM::BookingPromo;
  case MWMUTMDiscoverCatalogOnboarding: return UTM::DiscoverCatalogOnboarding;
  case MWMUTMFreeSamplesOnboading: return UTM::FreeSamplesOnboading;
  case MWMUTMOutdoorPlacepageGallery: return UTM::OutdoorPlacepageGallery;
  case MWMUTMGuidesOnMapGallery: return UTM::GuidesOnMapGallery;
  }
}

static inline UTMContent toUTMContent(MWMUTMContent content)
{
  switch (content)
  {
  case MWMUTMContentDescription: return UTMContent::Description;
  case MWMUTMContentView: return UTMContent::View;
  case MWMUTMContentDetails: return UTMContent::Details;
  case MWMUTMContentMore: return UTMContent::More;
  }
}
