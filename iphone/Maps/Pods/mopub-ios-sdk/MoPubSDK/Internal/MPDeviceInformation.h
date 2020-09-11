//
//  MPDeviceInformation.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPATSSetting.h"
#import "MPNetworkStatus.h"

NS_ASSUME_NONNULL_BEGIN

/**
 Read-only information pertaining to the current state of the device.
 */
@interface MPDeviceInformation : NSObject

/**
 The current App Transport Security settings of the device.
 */
@property (class, nonatomic, readonly) MPATSSetting appTransportSecuritySettings;

/**
 The current radio technology used by the device to connect to the internet.
 */
@property (class, nonatomic, readonly) MPNetworkStatus currentRadioAccessTechnology;

/**
 The currently cached carrier name.
 */
@property (class, nullable, nonatomic, copy, readonly) NSString * carrierName;

/**
 The currently cached carrier ISO country code.
 */
@property (class, nullable, nonatomic, copy, readonly) NSString * isoCountryCode;

/**
 The currently cached carrier mobile country code.
 */
@property (class, nullable, nonatomic, copy, readonly) NSString * mobileCountryCode;

/**
 The currently cached carrier mobile network code.
 */
@property (class, nullable, nonatomic, copy, readonly) NSString * mobileNetworkCode;

@end

NS_ASSUME_NONNULL_END
