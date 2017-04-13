//
//  MPNativeAd+Internal.h
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import "MPNativeAd.h"

@class MPNativeView;

@interface MPNativeAd (Internal)

@property (nonatomic, readonly) NSDate *creationDate;
@property (nonatomic) MPNativeView *associatedView;
@property (nonatomic, readwrite, strong) id<MPNativeAdRenderer> renderer;
@property (nonatomic, readonly) NSMutableSet *clickTrackerURLs;
@property (nonatomic, readonly) NSMutableSet *impressionTrackerURLs;
@property (nonatomic, readonly, strong) id<MPNativeAdAdapter> adAdapter;

/**
 * This method is called by the ad placers when the sizes of the ad placer stream
 * view's have changed. The ad placer will get the size from the renderer and just
 * pass it through to the mpnativead to update the view size since the ad is the only one
 * who has access to the ad view.
*/
- (void)updateAdViewSize:(CGSize)size;

/**
 * Retrieves the custom ad view with its frame set to the would-be containing native view. Unlike
 * `retrieveAdViewWithError:`, this method does not have side effects of changing the view hierarchy
 * and is only intended for size calculation purposes.
 *
 * @param error A pointer to an error object. If an error occurs, this pointer will be set to an
 * actual error object containing the error information.
 *
 * @return If successful, the method will return the rendered ad. The method will
 * return nil if it cannot render the ad data to a view.
 */
- (UIView *)retrieveAdViewForSizeCalculationWithError:(NSError **)error;

@end
