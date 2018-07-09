//
//  MPVASTCompanionAd.h
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MPVASTModel.h"

@interface MPVASTCompanionAd : MPVASTModel

@property (nonatomic, readonly) CGFloat assetHeight;
@property (nonatomic, readonly) CGFloat assetWidth;
@property (nonatomic, copy, readonly) NSURL *clickThroughURL;
@property (nonatomic, readonly) NSArray *clickTrackingURLs;
@property (nonatomic, readonly) CGFloat height;
@property (nonatomic, readonly) NSArray *HTMLResources;
@property (nonatomic, copy, readonly) NSString *identifier;
@property (nonatomic, readonly) NSArray *iframeResources;
@property (nonatomic, readonly) NSArray *staticResources;
@property (nonatomic, readonly) NSDictionary *trackingEvents;
@property (nonatomic, readonly) CGFloat width;

@end
