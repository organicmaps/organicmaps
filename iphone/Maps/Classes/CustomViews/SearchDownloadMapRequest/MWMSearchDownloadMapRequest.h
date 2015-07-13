//
//  MWMSearchDownloadMapRequest.h
//  Maps
//
//  Created by Ilya Grechuhin on 10.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MWMDownloadMapRequest.h"

@interface MWMSearchDownloadMapRequest : NSObject

- (nonnull instancetype)init __attribute__((unavailable("init is not available")));
- (nonnull instancetype)initWithParentView:(nonnull UIView *)parentView delegate:(nonnull id <MWMCircularProgressDelegate>)delegate;

- (void)showForLocationWithName:(nonnull NSString *)locationName mapSize:(nonnull NSString *)mapSize mapAndRouteSize:(nonnull NSString *)mapAndRouteSize download:(nonnull MWMDownloadMapRequestDownloadCallback)download select:(nonnull MWMDownloadMapRequestSelectCallback)select;
- (void)showForUnknownLocation:(nonnull MWMDownloadMapRequestSelectCallback)select;

- (void)startDownload;
- (void)stopDownload;
- (void)downloadProgress:(CGFloat)progress countryName:(nonnull NSString *)countryName;
- (void)setDownloadFailed;

@end
