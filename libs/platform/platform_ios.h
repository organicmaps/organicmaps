#pragma once

#include "platform/platform.hpp"

#import <Foundation/Foundation.h>

namespace platform
{
NSDictionary<NSString *, NSString *> * const kDeviceModelsBeforeMetalDriver = @{
  @"i386" : @"Simulator",
  @"iPad1,1" : @"iPad WiFi",
  @"iPad1,2" : @"iPad GSM",
  @"iPad2,1" : @"iPad 2 WiFi",
  @"iPad2,2" : @"iPad 2 GSM",
  @"iPad2,3" : @"iPad 2 GSM EV-DO",
  @"iPad2,4" : @"iPad 2",
  @"iPad2,5" : @"iPad Mini WiFi",
  @"iPad2,6" : @"iPad Mini GSM",
  @"iPad2,7" : @"iPad Mini CDMA",
  @"iPad3,1" : @"iPad 3rd gen. WiFi",
  @"iPad3,2" : @"iPad 3rd gen. GSM",
  @"iPad3,3" : @"iPad 3rd gen. CDMA",
  @"iPad3,4" : @"iPad 4th gen. WiFi",
  @"iPad3,5" : @"iPad 4th gen. GSM",
  @"iPad3,6" : @"iPad 4th gen. CDMA",
  @"iPad4,1" : @"iPad Air WiFi",
  @"iPad4,2" : @"iPad Air GSM",
  @"iPad4,3" : @"iPad Air CDMA",
  @"iPad4,4" : @"iPad Mini 2nd gen. WiFi",
  @"iPad4,5" : @"iPad Mini 2nd gen. GSM",
  @"iPad4,6" : @"iPad Mini 2nd gen. CDMA",
  @"iPad5,3" : @"iPad Air 2 WiFi",
  @"iPad5,4" : @"iPad Air 2 GSM",
  @"iPhone1,1" : @"iPhone",
  @"iPhone1,2" : @"iPhone 3G",
  @"iPhone2,1" : @"iPhone 3GS",
  @"iPhone3,1" : @"iPhone 4 GSM",
  @"iPhone3,2" : @"iPhone 4 CDMA",
  @"iPhone3,3" : @"iPhone 4 GSM EV-DO",
  @"iPhone4,1" : @"iPhone 4S",
  @"iPhone4,2" : @"iPhone 4S",
  @"iPhone4,3" : @"iPhone 4S",
  @"iPhone5,1" : @"iPhone 5",
  @"iPhone5,2" : @"iPhone 5",
  @"iPhone5,3" : @"iPhone 5c",
  @"iPhone5,4" : @"iPhone 5c",
  @"iPhone6,1" : @"iPhone 5s",
  @"iPhone6,2" : @"iPhone 5s",
  @"iPhone7,1" : @"iPhone 6 Plus",
  @"iPhone7,2" : @"iPhone 6",
  @"iPod1,1" : @"iPod Touch",
  @"iPod2,1" : @"iPod Touch 2nd gen.",
  @"iPod3,1" : @"iPod Touch 3rd gen.",
  @"iPod4,1" : @"iPod Touch 4th gen.",
  @"iPod5,1" : @"iPod Touch 5th gen.",
  @"x86_64" : @"Simulator"
};
NSDictionary<NSString *, NSString *> * const kDeviceModelsWithiOS10MetalDriver = @{
  @"iPad6,3" : @"iPad Pro (9.7 inch) WiFi",
  @"iPad6,4" : @"iPad Pro (9.7 inch) GSM",
  @"iPad6,7" : @"iPad Pro (12.9 inch) WiFi",
  @"iPad6,8" : @"iPad Pro (12.9 inch) GSM",
  @"iPhone8,1" : @"iPhone 6s",
  @"iPhone8,2" : @"iPhone 6s Plus",
  @"iPhone8,4" : @"iPhone SE"
};
NSDictionary<NSString *, NSString *> * const kDeviceModelsWithMetalDriver = @{
  @"iPad6,11" : @"iPad 5th gen. WiFi",
  @"iPad6,12" : @"iPad 5th gen. GSM",
  @"iPad7,1" : @"iPad Pro (12.9 inch) 2nd gen. WiFi",
  @"iPad7,2" : @"iPad Pro (12.9 inch) 2nd gen. GSM",
  @"iPad7,3" : @"iPad Pro (10.5-inch) WiFi",
  @"iPad7,4" : @"iPad Pro (10.5-inch) GSM",
  @"iPhone9,1" : @"iPhone 7",
  @"iPhone9,3" : @"iPhone 7",
  @"iPhone9,2" : @"iPhone 7 Plus",
  @"iPhone9,4" : @"iPhone 7 Plus",
  @"iPhone10,1" : @"iPhone 8",
  @"iPhone10,2" : @"iPhone 8 Plus",
  @"iPhone10,3" : @"iPhone X",
  @"iPhone10,4" : @"iPhone 8",
  @"iPhone10,5" : @"iPhone 8 Plus",
  @"iPhone10,6" : @"iPhone X",
};
}  // platform
