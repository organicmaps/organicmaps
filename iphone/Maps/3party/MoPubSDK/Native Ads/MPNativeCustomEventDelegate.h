//
//  MPNativeCustomEventDelegate.h
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

@class MPNativeAd;
@class MPNativeCustomEvent;

/**
 * Instances of your custom subclass of `MPNativeCustomEvent` will have an
 * `MPNativeCustomEventDelegate` delegate object. You use this delegate to communicate progress
 * (such as whether an ad has loaded successfully) back to the MoPub SDK.
 */
@protocol MPNativeCustomEventDelegate <NSObject>

/**
 * This method is called when the ad and all required ad assets are loaded.
 *
 * @param event You should pass `self` to allow the MoPub SDK to associate this event with the
 * correct instance of your custom event.
 * @param adObject An `MPNativeAd` object, representing the ad that was retrieved.
 */
- (void)nativeCustomEvent:(MPNativeCustomEvent *)event didLoadAd:(MPNativeAd *)adObject;

/**
 * This method is called when the ad or any required ad assets fail to load.
 *
 * @param event You should pass `self` to allow the MoPub SDK to associate this event with the
 * correct instance of your custom event.
 * @param error (*optional*) You may pass an error describing the failure.
 */
- (void)nativeCustomEvent:(MPNativeCustomEvent *)event didFailToLoadAdWithError:(NSError *)error;

@end
