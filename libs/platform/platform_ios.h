#pragma once

#include "platform/platform.hpp"

#import <Foundation/Foundation.h>

namespace platform
{
NSDictionary<NSString *, NSString *> * const kDeviceModelsWithiOS10MetalDriver = @{
  @"iPad6,3": @"iPad Pro (9.7 inch) WiFi",
  @"iPad6,4": @"iPad Pro (9.7 inch) GSM",
  @"iPad6,7": @"iPad Pro (12.9 inch) WiFi",
  @"iPad6,8": @"iPad Pro (12.9 inch) GSM",
  @"iPhone8,1": @"iPhone 6s",
  @"iPhone8,2": @"iPhone 6s Plus",
  @"iPhone8,4": @"iPhone SE"
};
NSDictionary<NSString *, NSString *> * const kDeviceModelsWithMetalDriver = @{
  @"iPad6,11": @"iPad 5th gen. WiFi",
  @"iPad6,12": @"iPad 5th gen. GSM",
  @"iPad7,1": @"iPad Pro (12.9 inch) 2nd gen. WiFi",
  @"iPad7,2": @"iPad Pro (12.9 inch) 2nd gen. GSM",
  @"iPad7,3": @"iPad Pro (10.5-inch) WiFi",
  @"iPad7,4": @"iPad Pro (10.5-inch) GSM",
  @"iPhone9,1": @"iPhone 7",
  @"iPhone9,3": @"iPhone 7",
  @"iPhone9,2": @"iPhone 7 Plus",
  @"iPhone9,4": @"iPhone 7 Plus",
  @"iPhone10,1": @"iPhone 8",
  @"iPhone10,2": @"iPhone 8 Plus",
  @"iPhone10,3": @"iPhone X",
  @"iPhone10,4": @"iPhone 8",
  @"iPhone10,5": @"iPhone 8 Plus",
  @"iPhone10,6": @"iPhone X",
};
}  // namespace platform
