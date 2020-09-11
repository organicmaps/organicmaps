//
//  MPNativeAd.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MPMoPubAd.h"
#import "MPNativeAdDelegate.h"

@protocol MPNativeAdAdapter;
@protocol MPNativeAdRenderer;
@class MPAdConfiguration;

/**
 * The `MPNativeAd` class is used to render and manage events for a native advertisement. The
 * class provides methods for accessing native ad properties returned by the server, as well as
 * convenience methods for URL navigation and metrics-gathering.
 */

@interface MPNativeAd : NSObject <MPMoPubAd>

/** @name Ad Resources */

/**
 * The delegate of the `MPNativeAd` object.
 */
@property (nonatomic, weak) id<MPNativeAdDelegate> delegate;

/**
 * A dictionary representing the native ad properties.
 */
@property (nonatomic, readonly) NSDictionary *properties;

- (instancetype)initWithAdAdapter:(id<MPNativeAdAdapter>)adAdapter;

/** @name Retrieving Ad View */

/**
 * Retrieves a rendered view containing the ad.
 *
 * @param error A pointer to an error object. If an error occurs, this pointer will be set to an
 * actual error object containing the error information.
 *
 * @return If successful, the method will return a view containing the rendered ad. The method will
 * return nil if it cannot render the ad data to a view.
 */
- (UIView *)retrieveAdViewWithError:(NSError **)error;

- (void)trackMetricForURL:(NSURL *)URL;

@end
