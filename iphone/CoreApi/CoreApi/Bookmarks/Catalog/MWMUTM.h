// These enumerations must correspond to C++ enumeration in partners_api/utm.hpp.
typedef NS_ENUM(NSInteger, MWMUTM) {
  MWMUTMNone = 0,
  MWMUTMBookmarksPageCatalogButton,
  MWMUTMToolbarButton,
  MWMUTMDownloadMwmBanner,
  MWMUTMLargeToponymsPlacepageGallery,
  MWMUTMSightseeingsPlacepageGallery,
  MWMUTMDiscoveryPageGallery,
  MWMUTMTipsAndTricks,
  MWMUTMBookingPromo,
  MWMUTMDiscoverCatalogOnboarding,
  MWMUTMFreeSamplesOnboading,
  MWMUTMOutdoorPlacepageGallery,
  MWMUTMGuidesOnMapGallery,
};

typedef NS_ENUM(NSInteger, MWMUTMContent) {
  MWMUTMContentDescription = 0,
  MWMUTMContentView,
  MWMUTMContentDetails,
  MWMUTMContentMore
};
