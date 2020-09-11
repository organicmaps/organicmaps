//
//  MPVASTCreative.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPVASTModel.h"

@class MPVASTLinearAd;
@class MPVASTCompanionAd;

@interface MPVASTCreative : MPVASTModel

@property (nonatomic, copy, readonly) NSString *identifier;
@property (nonatomic, copy, readonly) NSString *sequence;
@property (nonatomic, copy, readonly) NSString *adID;
@property (nonatomic, strong, readonly) MPVASTLinearAd *linearAd;
@property (nonatomic, strong, readonly) NSArray<MPVASTCompanionAd *> *companionAds;

@end
