//
//  MPVASTTrackingEvent.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPVASTModel.h"
#import "MPVideoEvent.h"

NS_ASSUME_NONNULL_BEGIN

@class MPVASTDurationOffset;

@interface MPVASTTrackingEvent : MPVASTModel

@property (nonatomic, copy, readonly) MPVideoEvent eventType;
@property (nonatomic, copy, readonly) NSURL *URL;
@property (nonatomic, readonly) MPVASTDurationOffset *progressOffset;

- (instancetype)initWithEventType:(MPVideoEvent)eventType
                              url:(NSURL *)url
                   progressOffset:(MPVASTDurationOffset * _Nullable)progressOffset;

@end

NS_ASSUME_NONNULL_END
