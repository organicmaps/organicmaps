//
//  MPBool.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

/**
 Tri-state boolean.
 */
typedef NS_ENUM(NSInteger, MPBool) {
    /**
     No
     */
    MPBoolNo = -1,

    /**
     Unknown
     */
    MPBoolUnknown = 0,

    /**
     Yes
     */
    MPBoolYes = 1,
};
