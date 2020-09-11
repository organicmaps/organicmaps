//
//  MPVASTResponse.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPVASTModel.h"

#import "MPVASTAd.h"
#import "MPVASTCompanionAd.h"
#import "MPVASTCreative.h"
#import "MPVASTDurationOffset.h"
#import "MPVASTIndustryIcon.h"
#import "MPVASTInline.h"
#import "MPVASTLinearAd.h"
#import "MPVASTMediaFile.h"
#import "MPVASTResource.h"
#import "MPVASTTrackingEvent.h"
#import "MPVASTWrapper.h"

@interface MPVASTResponse : MPVASTModel

@property (nonatomic, readonly) NSArray *ads;
@property (nonatomic, readonly) NSArray *errorURLs;
@property (nonatomic, copy, readonly) NSString *version;

@end
