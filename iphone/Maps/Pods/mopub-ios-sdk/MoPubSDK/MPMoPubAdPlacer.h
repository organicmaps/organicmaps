//
//  MPMoPubAdPlacer.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPMoPubAd.h"
#import "MPImpressionData.h"

NS_ASSUME_NONNULL_BEGIN

@protocol MPMoPubAdPlacerDelegate;

/**
 This protocol defines functionality that is shared between all MoPub ad placers.
 */
@protocol MPMoPubAdPlacer <NSObject>

@required
/**
 All MoPub ad placers have a delegate to call back when certain events occur.
 */
@property (nonatomic, weak, nullable) id<MPMoPubAdPlacerDelegate> delegate;

@end

/**
 This protocol defines callback events shared between all MoPub ad placers.
 */
@protocol MPMoPubAdPlacerDelegate <NSObject>

@optional
/**
 Called when an impression is fired on the @c MPMoPubAdPlacer instance. Includes
 information about the impression if applicable.

 @param adPlacer The @c MPMoPubAdPlacer instance that fired the impression
 @param ad The @c MPMoPubAd instance that fired the impression
 @param impressionData Information about the impression, or @c nil if the server didn't return any information.
 */
- (void)mopubAdPlacer:(id<MPMoPubAdPlacer>)adPlacer didTrackImpressionForAd:(id<MPMoPubAd>)ad withImpressionData:(MPImpressionData * _Nullable)impressionData;

@end

NS_ASSUME_NONNULL_END
