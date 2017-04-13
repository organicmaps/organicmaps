//
//  MPVASTCreative.h
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPVASTModel.h"

@class MPVASTLinearAd;

@interface MPVASTCreative : MPVASTModel

@property (nonatomic, copy, readonly) NSString *identifier;
@property (nonatomic, copy, readonly) NSString *sequence;
@property (nonatomic, copy, readonly) NSString *adID;
@property (nonatomic, readonly) MPVASTLinearAd *linearAd;
@property (nonatomic, readonly) NSArray *companionAds;

@end
