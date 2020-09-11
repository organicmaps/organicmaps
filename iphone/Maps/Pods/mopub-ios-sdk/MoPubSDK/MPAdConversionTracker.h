//
//  MPAdConversionTracker.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

#define MOPUB_CONVERSION_DEFAULTS_KEY @"com.mopub.conversion"
#define MOPUB_CONVERSION_APP_ID_KEY @"com.mopub.conversion.appId"

/**
 * The `MPAdConversionTracker` class provides a mechanism for reporting application download
 * (conversion) events to MoPub. This type of tracking is important for measuring the effectiveness
 * of cross-promotional and direct-sold advertising.
 *
 * To track application downloads, get a reference to the shared instance of this class using the
 * `sharedConversionTracker` method. Then, in your application delegate's
 * `application:didFinishLaunchingWithOptions:` method, call the
 * `reportApplicationOpenForApplicationID:` method on the shared instance. With this call in place,
 * the conversion tracker will report an event to MoPub whenever the application is launched on a
 * given device for the first time. Any subsequent launches will not be recorded as conversion
 * events.
 */

@interface MPAdConversionTracker : NSObject

/** @name Recording Conversions */

/**
 * Returns the shared instance of the `MPAdConversionTracker` class.
 *
 * @return A singleton `MPAdConversionTracker` object.
 */
+ (MPAdConversionTracker *)sharedConversionTracker;

/**
 * Notifies MoPub that a conversion event should be recorded for the application corresponding to
 * the specified `appID`.
 *
 * A conversion event will only be reported once per application download, even if this method
 * is called multiple times.
 *
 * @param appID An iTunes application ID.
 *
 * The easiest way to find the correct ID for your application is to generate an iTunes URL using
 * the [iTunes Link Maker](https://itunes.apple.com/linkmaker), and then extract the number
 * immediately following the "id" string.
 *
 *     For example, the iTunes URL for the "Find My Friends" application is
 *     https://itunes.apple.com/us/app/find-my-friends/id466122094, so its application ID is
 *     466122094.
 */
- (void)reportApplicationOpenForApplicationID:(NSString *)appID;

@end
