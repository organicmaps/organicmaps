//
//  MPAdServerURLBuilder.h
//  MoPub
//
//  Copyright (c) 2012 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

@interface MPAdServerURLBuilder : NSObject

+ (NSURL *)URLWithAdUnitID:(NSString *)adUnitID
                  keywords:(NSString *)keywords
                  location:(CLLocation *)location
                   testing:(BOOL)testing;

+ (NSURL *)URLWithAdUnitID:(NSString *)adUnitID
                  keywords:(NSString *)keywords
                  location:(CLLocation *)location
      versionParameterName:(NSString *)versionParameterName
                   version:(NSString *)version
                   testing:(BOOL)testing
             desiredAssets:(NSArray *)assets;

+ (NSURL *)URLWithAdUnitID:(NSString *)adUnitID
                  keywords:(NSString *)keywords
                  location:(CLLocation *)location
      versionParameterName:(NSString *)versionParameterName
                   version:(NSString *)version
                   testing:(BOOL)testing
             desiredAssets:(NSArray *)assets
                adSequence:(NSInteger)adSequence;

@end
