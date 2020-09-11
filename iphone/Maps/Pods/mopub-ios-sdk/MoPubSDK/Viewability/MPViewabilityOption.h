//
//  MPViewabilityOption.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
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
