//
//  MPVASTAd.h
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPVASTModel.h"

@class MPVASTInline;
@class MPVASTWrapper;

@interface MPVASTAd : MPVASTModel

@property (nonatomic, copy, readonly) NSString *identifier;
@property (nonatomic, copy, readonly) NSString *sequence;
@property (nonatomic, readonly) MPVASTInline *inlineAd;
@property (nonatomic, readonly) MPVASTWrapper *wrapper;

@end
