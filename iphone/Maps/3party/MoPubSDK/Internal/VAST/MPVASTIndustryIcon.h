//
//  MPVASTIndustryIcon.h
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MPVASTModel.h"

@class MPVASTDurationOffset;
@class MPVASTResource;

@interface MPVASTIndustryIcon : MPVASTModel

@property (nonatomic, copy, readonly) NSString *program;
@property (nonatomic, readonly) CGFloat height;
@property (nonatomic, readonly) CGFloat width;
@property (nonatomic, copy, readonly) NSString *xPosition;
@property (nonatomic, copy, readonly) NSString *yPosition;

@property (nonatomic, copy, readonly) NSString *apiFramework;
@property (nonatomic, readonly) NSTimeInterval duration;
@property (nonatomic, readonly) MPVASTDurationOffset *offset;

@property (nonatomic, copy, readonly) NSURL *clickThroughURL;
@property (nonatomic, readonly) NSArray *clickTrackingURLs;
@property (nonatomic, readonly) NSArray *viewTrackingURLs;

@property (nonatomic, readonly) MPVASTResource *HTMLResource;
@property (nonatomic, readonly) MPVASTResource *iframeResource;
@property (nonatomic, readonly) MPVASTResource *staticResource;

@end
