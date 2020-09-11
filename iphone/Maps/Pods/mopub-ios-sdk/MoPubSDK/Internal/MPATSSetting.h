//
//  MPATSSetting.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

typedef NS_OPTIONS(NSUInteger, MPATSSetting) {
    MPATSSettingEnabled = 0,
    MPATSSettingAllowsArbitraryLoads = (1 << 0),
    MPATSSettingAllowsArbitraryLoadsForMedia = (1 << 1),
    MPATSSettingAllowsArbitraryLoadsInWebContent = (1 << 2),
    MPATSSettingRequiresCertificateTransparency = (1 << 3),
    MPATSSettingAllowsLocalNetworking = (1 << 4),
};
