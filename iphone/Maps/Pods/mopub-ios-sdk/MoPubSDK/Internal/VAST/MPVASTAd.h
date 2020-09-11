//
//  MPVASTAd.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
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
