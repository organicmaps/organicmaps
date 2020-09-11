//
//  MPNativeCustomEvent.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPNativeCustomEventDelegate.h"

/**
 * The MoPub iOS SDK mediates third-party native ad networks using custom events. Custom events are
 * responsible for instantiating and manipulating native ad objects in the third-party SDK and
 * translating and communicating events from those objects back to the MoPub SDK by notifying a
 * delegate.
 *
 * `MPNativeCustomEvent` is a base class for custom events that support native ads. By implementing
 * subclasses of `MPNativeCustomEvent` you can enable the MoPub SDK to support a variety of
 * third-party ad networks.
 *
 * Your implementation should create an `MPNativeAd` object using an appropriate `MPNativeAdAdapter`
 * for your network. Your custom event should also call the appropriate
 * `MPNativeCustomEventDelegate` methods.
 *
 * At runtime, the MoPub SDK will find and instantiate an `MPNativeCustomEvent` subclass as needed
 * and invoke its `-requestAdWithCustomEventInfo:` method.
 */
@interface MPNativeCustomEvent : NSObject

/** @name Requesting a Native Ad */

/**
 * Called when the MoPub SDK requires a new native ad.
 *
 * When the MoPub SDK receives a response indicating it should load a custom event, it will send
 * this message to your custom event class. Your implementation should load a native ad from a
 * third-party ad network.
 *
 * @param info A dictionary containing additional custom data associated with a given custom event
 * request. This data is configurable on the MoPub website, and may be used to pass dynamic
 * information, such as publisher IDs.
 * @param adMarkup An optional ad markup to use.
 */
- (void)requestAdWithCustomEventInfo:(NSDictionary *)info adMarkup:(NSString *)adMarkup;

/** @name Caching Image Resources */

/**
 * Downloads and pre-caches images.
 *
 * If your ad network does not provide built-in support for image caching, you may invoke this
 * method in your custom event implementation to pre-cache image assets. If you do call this method,
 * you should wait until the completion block is called before invoking the appropriate
 * success or failure callbacks on the `MPNativeCustomEventDelegate`.
 *
 * @param imageURLs An array of `NSURL` objects representing image resource locations.
 * @param completionBlock A block that will be called after all download operations are complete.
 * If any image downloads do not complete successfully, the `errors` parameter will contain
 * error information about the failures.
 */
- (void)precacheImagesWithURLs:(NSArray *)imageURLs completionBlock:(void (^)(NSArray *errors))completionBlock;

/** @name Communicating with the MoPub SDK */

/**
 * The `MPNativeCustomEventDelegate` receives messages concerning the status of loading a native ad.
 *
 * The `delegate` object defines several methods that you should call in order to inform MoPub
 * of the progress of your custom event.
 */
@property (nonatomic, weak) id<MPNativeCustomEventDelegate> delegate;

/**
 * An optional dictionary containing extra local data.
 */
@property (nonatomic, copy) NSDictionary *localExtras;

@end
