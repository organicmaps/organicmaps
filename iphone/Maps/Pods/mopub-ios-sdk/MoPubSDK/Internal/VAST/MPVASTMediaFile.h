//
//  MPVASTMediaFile.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MPVASTModel.h"

@interface MPVASTMediaFile : MPVASTModel

@property (nonatomic, copy, readonly) NSString *identifier;
@property (nonatomic, copy, readonly) NSString *delivery;
@property (nonatomic, copy, readonly) NSString *mimeType;
@property (nonatomic, readonly) CGFloat bitrate;
@property (nonatomic, readonly) CGFloat width;
@property (nonatomic, readonly) CGFloat height;
@property (nonatomic, copy, readonly) NSURL *URL;

@end

@interface MPVASTMediaFile (Selection)

/**
 Pick the best media file that fits into the provided container size and scale factor.
 */
+ (MPVASTMediaFile *)bestMediaFileFromCandidates:(NSArray<MPVASTMediaFile *> *)candidates
                                forContainerSize:(CGSize)containerSize
                            containerScaleFactor:(CGFloat)containerScaleFactor;

@end
