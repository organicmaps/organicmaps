//
//  MPVASTInline.h
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPVASTModel.h"

@interface MPVASTInline : MPVASTModel

@property (nonatomic, readonly) NSArray *creatives;
@property (nonatomic, readonly) NSArray *errorURLs;
@property (nonatomic, readonly) NSDictionary *extensions;
@property (nonatomic, readonly) NSArray *impressionURLs;

@end
