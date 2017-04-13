//
//  MPVASTResource.h
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPVASTModel.h"

@interface MPVASTResource : MPVASTModel

@property (nonatomic, readonly) NSString *content;
@property (nonatomic, readonly) NSString *staticCreativeType;

@end
