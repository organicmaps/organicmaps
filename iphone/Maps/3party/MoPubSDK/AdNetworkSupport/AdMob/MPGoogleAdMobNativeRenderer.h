#if __has_include(<MoPub / MoPub.h>)
#import <MoPub/MoPub.h>
#else
#import "MPNativeAdRenderer.h"
#import "MPNativeAdRendererSettings.h"
#endif

@class MPNativeAdRendererConfiguration;
@class MPStaticNativeAdRendererSettings;

@interface MPGoogleAdMobNativeRenderer : NSObject<MPNativeAdRendererSettings>

/// The viewSizeHandler is used to allow the app to configure its native ad view size.
@property(nonatomic, readwrite, copy) MPNativeViewSizeHandler viewSizeHandler;

/// Constructs and returns an MPNativeAdRendererConfiguration object specific for the
/// MPGoogleAdMobNativeRenderer. You must set all the properties on the configuration object.
/// @param rendererSettings Application defined settings.
/// @return A configuration object for MPGoogleAdMobNativeRenderer.
+ (MPNativeAdRendererConfiguration *)rendererConfigurationWithRendererSettings:
    (id<MPNativeAdRendererSettings>)rendererSettings;

@end
