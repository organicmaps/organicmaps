//
//  MPBannerCustomEvent+Internal.h
//  MoPubSampleApp
//
//  Copyright © 2017 MoPub. All rights reserved.
//

#import "MPBannerCustomEvent.h"

@interface MPBannerCustomEvent (Internal)

/**
 * Track marketplace and 3rd party impressions.
 * Extended class implements this method if necessary.
 * Currently, only HTML and MRAID banners contain MPX and 3rd party trackers.
 * Mediated networks don't contain MPX and 3rd party trackers.
 */

- (void)trackMPXAndThirdPartyImpressions;

/**
 * Start viewability tracker. The default implementation of this method does nothing.
 * Subclasses can override this method if necessary.
 */
- (void)startViewabilityTracker;

@end
