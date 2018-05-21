//
//  MPViewabilityOption.h
//  MoPubSDK
//
//  Copyright © 2018 MoPub. All rights reserved.
//

/**
 * Available viewability options
 * @remark Any changes made to this bitmask should also be reflected in `MPViewabilityTracker`
 * `+ (void)initialize` method.
 */
typedef NS_OPTIONS(NSInteger, MPViewabilityOption) {
    MPViewabilityOptionNone = 0,
    MPViewabilityOptionIAS  = 1 << 0,
    MPViewabilityOptionMoat = 1 << 1,
    MPViewabilityOptionAll  = ((MPViewabilityOptionMoat << 1) - 1)
};
