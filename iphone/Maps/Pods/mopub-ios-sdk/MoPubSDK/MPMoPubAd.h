//
//  MPMoPubAd.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPImpressionData.h"

NS_ASSUME_NONNULL_BEGIN

@protocol MPMoPubAdDelegate;

/**
 This protocol defines functionality that is shared between all MoPub ads.
 */
@protocol MPMoPubAd <NSObject>

@required
/**
 All MoPub ads have a delegate to call back when certain events occur.
 */
@property (nonatomic, weak, nullable) id<MPMoPubAdDelegate> delegate;

@end

/**
 This protocol defines callback events shared between all MoPub ads.
 */
@protocol MPMoPubAdDelegate <NSObject>

@optional
/**
 Called when an impression is fired on the @c MPMoPubAd instance. Includes information about the impression if applicable.

 @param ad The @c MPMoPubAd instance that fired the impression
 @param impressionData Information about the impression, or @c nil if the server didn't return any information.
 */
- (void)mopubAd:(id<MPMoPubAd>)ad didTrackImpressionWithImpressionData:(MPImpressionData * _Nullable)impressionData;

@end

NS_ASSUME_NONNULL_END
