//
//  MPNativeAdRequest.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

@class MPNativeAd;
@class MPNativeAdRequest;
@class MPNativeAdRequestTargeting;

typedef void(^MPNativeAdRequestHandler)(MPNativeAdRequest *request,
                                      MPNativeAd *response,
                                      NSError *error);

////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * The `MPNativeAdRequest` class is used to manage individual requests to the MoPub ad server for
 * native ads.
 *
 * @warning **Note:** This class is meant for one-off requests for which you intend to manually
 * process the native ad response. If you are using `MPTableViewAdPlacer` or
 * `MPCollectionViewAdPlacer` to display ads, there should be no need for you to use this class.
 */

@interface MPNativeAdRequest : NSObject

/** @name Targeting Information */

/**
 * An object representing targeting parameters that can be passed to the MoPub ad server to
 * serve more relevant advertising.
 */
@property (nonatomic, strong) MPNativeAdRequestTargeting *targeting;

/** @name Initializing and Starting an Ad Request */

/**
 * Initializes a request object.
 *
 * @param identifier The ad unit identifier for this request. An ad unit is a defined placement in
 * your application set aside for advertising. Ad unit IDs are created on the MoPub website.
 *
 * @param rendererConfigurations An array of MPNativeAdRendererConfiguration objects that control how
 * the native ad is rendered.
 *
 * @return An `MPNativeAdRequest` object.
 */
+ (MPNativeAdRequest *)requestWithAdUnitIdentifier:(NSString *)identifier rendererConfigurations:(NSArray *)rendererConfigurations;

/**
 * Executes a request to the MoPub ad server.
 *
 * @param handler A block to execute when the request finishes. The block includes as parameters the
 * request itself and either a valid MPNativeAd or an NSError object indicating failure.
 */
- (void)startWithCompletionHandler:(MPNativeAdRequestHandler)handler;

@end
